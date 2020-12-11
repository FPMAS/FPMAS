#include "model.h"

namespace fpmas {
	namespace model {
		namespace detail {

			const DefaultBehavior AgentGroupBase::default_behavior;

			void InsertAgentNodeCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.getMpiCommunicator().getRank(),
						"INSERT_AGENT_CALLBACK", "Inserting agent %s in graph.", FPMAS_C_STR(node->getId()));
				agent->setNode(node);
				agent->setModel(&model);

				for(auto gid : agent->groupIds())
					model.getGroup(gid).insert(&agent);
			}

			void EraseAgentNodeCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.getMpiCommunicator().getRank(),
						"ERASE_AGENT_CALLBACK", "Erasing agent %s from graph.", FPMAS_C_STR(node->getId()));
				for(auto group : agent->groups()) {
					group->erase(&agent);
				}
			}

			void SetAgentLocalCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.getMpiCommunicator().getRank(),
						"SET_AGENT_LOCAL_CALLBACK", "Setting agent %s LOCAL.", FPMAS_C_STR(node->getId()));
				for(auto group : agent->groups())
					group->agentExecutionJob().add(*agent->task(group->groupId()));
			}

			void SetAgentDistantCallback::call(AgentNode *node) {
				api::model::AgentPtr& agent = node->data();
				FPMAS_LOGD(model.getMpiCommunicator().getRank(),
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

			void Model::insert(api::model::GroupId id, api::model::AgentGroup* group) {
				_groups.insert({id, group});
			}

			fpmas::api::model::AgentGroup& Model::getGroup(api::model::GroupId id) const {
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

			void Model::removeGroup(api::model::AgentGroup& group) {
				group.clear();
				_groups.erase(group.groupId());
				delete &group;
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

			AgentGroupBase::AgentGroupBase(GroupId group_id, api::model::AgentGraph& agent_graph)
				: AgentGroupBase(group_id, agent_graph, default_behavior) {
				}

			AgentGroupBase::AgentGroupBase(
					GroupId group_id,
					api::model::AgentGraph& agent_graph,
					const api::model::Behavior& behavior)
				: id(group_id), agent_graph(agent_graph), job_base(), sync_graph_task(agent_graph), _behavior(behavior) {
					job_base.setEndTask(sync_graph_task);
				}

			void AgentGroupBase::add(api::model::Agent* agent) {
				agent->addGroupId(id);

				if(agent->groups().empty()) {
					// The agent does not belong to any group yet, so we insert
					// it in the graph.
					// InsertAgentNodeCallback is called.
					agent_graph.buildNode(api::model::AgentPtr {agent});
				} else {
					// The agent as already been added to a group, so it is
					// already contained in the graph
					//
					// This is necessarily performed on a LOCAL agent: adding a
					// DISTANT agent to a group is not allowed.
					assert(agent->node()->state() == graph::LocationState::LOCAL);

					// The node is already built and inserted in the graph, so
					// we just need to insert is in this new group. A new task
					// corresponding to this group is bound to the agent in
					// this process
					this->insert(&agent->node()->data());

					// Because of the requirements of this method, agent is
					// LOCAL, so it must be scheduled for execution in this
					// group.
					// Notice that, since the agent was already added to the
					// group, SetAgentLocalCallback will not be called in this
					// case.
					this->agentExecutionJob().add(*agent->task(this->groupId()));
				}
			}

			void AgentGroupBase::remove(api::model::Agent* agent) {
				if(agent->node()->state() == graph::LocationState::LOCAL) {
					// Unschedule agent task. If the node is DISTANT, task was already
					// unscheduled.
					// This must be performed before erase(), since it will
					// clear and delete the associated agent's task.
					this->agentExecutionJob().remove(*agent->task(this->groupId()));
				}

				this->erase(&agent->node()->data());

				if(agent->node()->state() == graph::LocationState::LOCAL) {
					// If the agent is not contained into any group, delete it
					// from the graph
					if(agent->groups().empty())
						agent_graph.removeNode(agent->node());
				}
			}

			void AgentGroupBase::insert(api::model::AgentPtr* agent) {
				// Inserts agent into the internal agents() list
				_agents.push_back(agent);

				// Inserts this group to Agent::groups()
				agent->get()->addGroup(this);
				// addGroupId must not be called there, since:
				// 1. it has already been called from AgentGroupBase::add()
				// 2. else, the agent have been imported, and its group Ids
				// list was already up to date (what allows to effectively
				// insert it in the group (see InsertAgentNodeCallback)

				// Binds a task corresponding to this group behavior to the
				// agent
				// The task is not added to agentExecutionJob(), since this
				// must be handled by SetAgentLocalCallback (or by the add()
				// method, see above)
				AgentBehaviorTask* task
					= new AgentBehaviorTask(this->behavior(), *agent);
				agent->get()->setTask(this->groupId(), task);
			}

			void AgentGroupBase::erase(api::model::AgentPtr* agent) {
				// Erases agent from the local agents() list
				_agents.erase(std::remove(_agents.begin(), _agents.end(), agent));
	
				// The task is not removed from agentExecutionJob(), since this
				// must be handled by SetAgentDistantCallback (or by the remove()
				// method, see above)
				delete agent->get()->task(this->groupId());

				// Erases this group from Agent::groups(), and erases the entry
				// corresponding to this group in Agent::tasks() (that's why
				// the task is deleted before)
				agent->get()->removeGroup(this);
				// Erases this group from Agent::groupIds()
				agent->get()->removeGroupId(this->groupId());
			}

			void AgentGroupBase::clear() {
				for(auto agent : this->agents())
					this->remove(agent);
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
