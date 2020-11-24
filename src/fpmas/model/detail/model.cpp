#include "model.h"

namespace fpmas {
	namespace model {
		namespace detail {

			const DefaultBehavior AgentGroupBase::default_behavior;

			void InsertAgentNodeCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
						"INSERT_AGENT_CALLBACK", "Inserting agent %s in graph.", FPMAS_C_STR(node->getId()));
				for(auto gid : agent->groupIds())
					agent->addGroup(&model.getGroup(gid));
				for(auto group : agent->groups())
					group->insert(&agent);
				agent->setNode(node);
				agent->setModel(&model);
				for(auto group : agent->groups()) {
					AgentBehaviorTask* task
						= new AgentBehaviorTask(group->behavior(), agent);
					agent->setTask(group->groupId(), task);
				}
			}

			void EraseAgentNodeCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
						"ERASE_AGENT_CALLBACK", "Erasing agent %s from graph.", FPMAS_C_STR(node->getId()));
				if(node->state() == graph::LocationState::LOCAL) {
					// Unschedule agent task. If the node is DISTANT, task was already
					// unscheduled.
					for(auto group : agent->groups())
						group->agentExecutionJob().remove(*agent->task(group->groupId()));
				}
				for(auto group : agent->groups())
					group->erase(&agent);
				for(auto task : agent->tasks())
					delete task.second;
			}

			void SetAgentLocalCallback::call(AgentNode *node) {
				api::model::Agent* agent = node->data();
				FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
						"SET_AGENT_LOCAL_CALLBACK", "Setting agent %s LOCAL.", FPMAS_C_STR(node->getId()));
				for(auto group : agent->groups())
					group->agentExecutionJob().add(*agent->task(group->groupId()));
			}

			void SetAgentDistantCallback::call(AgentNode *node) {
				api::model::Agent* agent = node->data();
				FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
						"SET_AGENT_DISTANT_CALLBACK", "Setting agent %s DISTANT.", FPMAS_C_STR(node->getId()));
				// Unschedule agent task 
				for(auto group : agent->groups())
					group->agentExecutionJob().remove(*agent->task(group->groupId()));
			}

			Model::Model(
					api::model::AgentGraph& graph,
					api::scheduler::Scheduler& scheduler,
					api::runtime::Runtime& runtime,
					LoadBalancingAlgorithm& load_balancing)
				: _graph(graph), _scheduler(scheduler), _runtime(runtime), _loadBalancingJob(),
				load_balancing_task(_graph, load_balancing) {
					_loadBalancingJob.add(load_balancing_task);
					_graph.addCallOnInsertNode(insert_node_callback);
					_graph.addCallOnEraseNode(erase_node_callback);
					_graph.addCallOnSetLocal(set_local_callback);
					_graph.addCallOnSetDistant(set_distant_callback);
				}

			Model::~Model() {
				_graph.clear();
				for(auto group : _groups)
					delete group.second;
			}

			void Model::insert(GroupId id, api::model::AgentGroup* group) {
				_groups.insert({id, group});
			}


			api::model::AgentGroup& Model::getGroup(api::model::GroupId id) const {
				return *_groups.at(id);
			}

			api::model::AgentGroup& Model::buildGroup(api::model::GroupId id) {
				api::model::AgentGroup* group = new AgentGroup(id, _graph);
				this->insert(id, group); 
				return *group;
			}

			api::model::AgentGroup& Model::buildGroup(api::model::GroupId id, const api::model::Behavior& behavior) {
				api::model::AgentGroup* group = new AgentGroup(id, _graph, behavior);
				this->insert(id, group); 
				return *group;
			}

			AgentEdge* Model::link(api::model::Agent *src_agent, api::model::Agent *tgt_agent, api::graph::LayerId layer) {
				return _graph.link(src_agent->node(), tgt_agent->node(), layer);
			}

			void Model::unlink(api::model::AgentEdge *edge) {
				_graph.unlink(edge);
			}

			void LoadBalancingTask::run() {
				agent_graph.balance(load_balancing);
			}

			AgentGroupBase::AgentGroupBase(api::model::GroupId group_id, api::model::AgentGraph& agent_graph)
				: AgentGroupBase(group_id, agent_graph, default_behavior) {
				}

			AgentGroupBase::AgentGroupBase(
					api::model::GroupId group_id,
					api::model::AgentGraph& agent_graph,
					const api::model::Behavior& behavior)
				: id(group_id), agent_graph(agent_graph), job_base(), sync_graph_task(agent_graph), _behavior(behavior) {
					job_base.setEndTask(sync_graph_task);
				}

			void AgentGroupBase::add(api::model::Agent* agent) {
				agent->addGroupId(id);
				// TODO: to improve in 2.0
				if(agent->groups().empty()) {
					agent_graph.buildNode(agent);
				}
				else {
					// It is assumed that the agent has already been added to a
					// group, and so the "insert node" and "set local"
					// callbacks above have already been triggered, for the
					// first group to which the agent was added.
					//
					// Since those callbacks are not triggered again for next
					// groups, we can manually set up group and task for the agent.
					//
					// This might be modified in version 2.0
					//
					// Notice that this is only valid when agents are "added"
					// to the group, callbacks work as expected when the agent
					// is "inserted" when migration are performed for example.

					agent->addGroup(this);

					// The node is already built and inserted in the graph, so
					// this is safe (but hacky)
					this->insert(&agent->node()->data());
					AgentBehaviorTask* task
						= new AgentBehaviorTask(this->behavior(), agent->node()->data()); // Same as above
					agent->setTask(this->groupId(), task);

					this->job().add(*task);
				}
			}

			void AgentGroupBase::remove(api::model::Agent* agent) {
				agent_graph.removeNode(agent->node());
			}

			void AgentGroupBase::insert(api::model::AgentPtr* agent) {
				_agents.push_back(agent);
			}

			void AgentGroupBase::erase(api::model::AgentPtr* agent) {
				_agents.erase(std::remove(_agents.begin(), _agents.end(), agent));
			}

			std::vector<api::model::Agent*> AgentGroupBase::agents() const {
				std::vector<api::model::Agent*> agents;
				for(auto agent : _agents)
					agents.push_back(*agent);
				return agents;
			}

			std::vector<api::model::Agent*> AgentGroupBase::localAgents() const {
				std::vector<api::model::Agent*> local_agents;
				for(auto agent : _agents)
					if(agent->get()->node()->state() == graph::LocationState::LOCAL)
						local_agents.push_back(*agent);
				return local_agents;
			}

		}
	}
}
