#include "model.h"
#include "fpmas/graph/distributed_node.h"

namespace fpmas { namespace model {

	const JID Model::LB_JID = 0;

	AgentGroup::AgentGroup(GroupId group_id, AgentGraph& agent_graph, JID job_id)
		: id(group_id), agent_graph(agent_graph), _job(job_id), sync_graph_task(agent_graph) {
			_job.setEndTask(sync_graph_task);
	}

	void AgentGroup::add(api::model::Agent* agent) {
		agent->setGroupId(id);
		agent_graph.buildNode(agent);
		_agents.push_back(agent);
	}

	void AgentGroup::remove(api::model::Agent* agent) {
		agent_graph.removeNode(agent->node());
		_agents.erase(std::remove(_agents.begin(), _agents.end(), agent));
	}

	void InsertAgentCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"INSERT_AGENT_CALLBACK", "Inserting agent %s in graph.", ID_C_STR(node->getId()));
		agent->setGroup(&model.getGroup(agent->groupId()));
		agent->setNode(node);
		agent->setGraph(&model.graph());
		AgentTask* task = new AgentTask;
		task->setAgent(agent);
		agent->setTask(task);
	}

	void EraseAgentCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"ERASE_AGENT_CALLBACK", "Erasing agent %s from graph.", ID_C_STR(node->getId()));
		// Deletes task and agent
		if(node->state() == graph::LocationState::LOCAL) {
			// Unschedule agent task. If the node is DISTANT, task was already
			// unscheduled.
			agent->group()->job().remove(*agent->task());
			//model.getGroup(agent->groupId()).job().remove(*agent->task());
		}
		delete agent->task();
	}

	void SetAgentLocalCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"SET_AGENT_LOCAL_CALLBACK", "Setting agent %s LOCAL.", ID_C_STR(node->getId()));
		model.getGroup(agent->groupId()).job().add(*agent->task());
	}

	void SetAgentDistantCallback::call(AgentNode *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(model.graph().getMpiCommunicator().getRank(),
				"SET_AGENT_DISTANT_CALLBACK", "Setting agent %s DISTANT.", ID_C_STR(node->getId()));
		// Unschedule agent task 
		model.getGroup(agent->groupId()).job().remove(*agent->task());
	}

	Model::Model(
			AgentGraph& graph,
			api::scheduler::Scheduler& scheduler,
			api::runtime::Runtime& runtime,
			LoadBalancingAlgorithm& load_balancing)
		: gid(0), _graph(graph), _scheduler(scheduler), _runtime(runtime), _loadBalancingJob(LB_JID),
		load_balancing_task(_graph, load_balancing, _scheduler, _runtime) {
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

	AgentGroup& Model::getGroup(GroupId id) const {
		return static_cast<AgentGroup&>(*_groups.at(id));
	}

	AgentGroup& Model::buildGroup() {
		AgentGroup* group = new AgentGroup(gid, _graph, job_id++);
		_groups.insert({gid, group}); 
		gid++;
		return *group;
	}

	void LoadBalancingTask::run() {
		agent_graph.balance(load_balancing);
	}
}}
