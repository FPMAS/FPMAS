#include "model.h"
#include "fpmas/graph/distributed_node.h"

namespace fpmas { namespace model {

	AgentGroup::AgentGroup(api::model::GroupId group_id, api::model::AgentGraph& agent_graph)
		: id(group_id), agent_graph(agent_graph), _job(), sync_graph_task(agent_graph) {
			_job.setEndTask(sync_graph_task);
	}

	void AgentGroup::add(api::model::Agent* agent) {
		agent->setGroupId(id);
		agent_graph.buildNode(agent);
	}

	void AgentGroup::remove(api::model::Agent* agent) {
		agent_graph.removeNode(agent->node());
	}

	void AgentGroup::insert(api::model::AgentPtr* agent) {
		_agents.push_back(agent);
	}

	void AgentGroup::erase(api::model::AgentPtr* agent) {
		_agents.erase(std::remove(_agents.begin(), _agents.end(), agent));
	}

	std::vector<api::model::AgentPtr*> AgentGroup::localAgents() const {
		std::vector<api::model::AgentPtr*> local_agents;
		for(auto agent : _agents)
			if(agent->get()->node()->state() == graph::LocationState::LOCAL)
				local_agents.push_back(agent);
		return local_agents;
	}

	void InsertAgentNodeCallback::call(AgentNode *node) {
		api::model::AgentPtr& agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"INSERT_AGENT_CALLBACK", "Inserting agent %s in graph.", FPMAS_C_STR(node->getId()));
		agent->setGroup(&model.getGroup(agent->groupId()));
		agent->group()->insert(&agent);
		agent->setNode(node);
		agent->setModel(&model);
		AgentTask* task = new AgentTask(agent);
		//task->setAgent(agent);
		agent->setTask(task);
	}

	void EraseAgentNodeCallback::call(AgentNode *node) {
		api::model::AgentPtr& agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"ERASE_AGENT_CALLBACK", "Erasing agent %s from graph.", FPMAS_C_STR(node->getId()));
		// Deletes task and agent
		if(node->state() == graph::LocationState::LOCAL) {
			// Unschedule agent task. If the node is DISTANT, task was already
			// unscheduled.
			agent->group()->job().remove(*agent->task());
			//model.getGroup(agent->groupId()).job().remove(*agent->task());
		}
		agent->group()->erase(&agent);
		delete agent->task();
	}

	void SetAgentLocalCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"SET_AGENT_LOCAL_CALLBACK", "Setting agent %s LOCAL.", FPMAS_C_STR(node->getId()));
		model.getGroup(agent->groupId()).job().add(*agent->task());
	}

	void SetAgentDistantCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"SET_AGENT_DISTANT_CALLBACK", "Setting agent %s DISTANT.", FPMAS_C_STR(node->getId()));
		// Unschedule agent task 
		model.getGroup(agent->groupId()).job().remove(*agent->task());
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

	AgentGroup& Model::getGroup(api::model::GroupId id) const {
		return static_cast<AgentGroup&>(*_groups.at(id));
	}

	AgentGroup& Model::buildGroup(api::model::GroupId id) {
		AgentGroup* group = new AgentGroup(id, _graph);
		_groups.insert({id, group}); 
		return *group;
	}

	void LoadBalancingTask::run() {
		agent_graph.balance(load_balancing);
	}
}}
