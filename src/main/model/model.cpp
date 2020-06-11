#include "model.h"
#include "graph/parallel/distributed_node.h"

namespace FPMAS::model {

	const JID Model::LB_JID = 0;

	AgentGroup::AgentGroup(GroupId group_id, AgentGraph& agent_graph, JID job_id)
		: id(group_id), agent_graph(agent_graph), _job(job_id), sync_graph_task(agent_graph) {
			_job.setEndTask(sync_graph_task);
	}

	void AgentGroup::add(api::model::Agent* agent) {
		//AgentTask*& task = agent_tasks.emplace_back(new AgentTask(*agent));
		agent_graph.buildNode(agent);
	}

	void AgentGroup::remove(api::model::Agent* agent) {
		agent_graph.removeNode(agent->node());
	}

	void InsertNodeCallback::call(api::graph::parallel::DistributedNode<api::model::Agent *> *node) {
		api::model::Agent*& agent = node->data();
		FPMAS_LOGD(-1, "[INSERT_NODE_CALLBACK]", "Inserting agent %s in graph.", ID_C_STR(node->getId()));
		agent->setNode(node);
		agent->setTask(new AgentTask(*agent));
		model.getGroup(agent->groupId()).job().add(*agent->task());
	}

	void EraseNodeCallback::call(api::graph::parallel::DistributedNode<api::model::Agent *> *node) {
		api::model::Agent* agent = node->data();
		FPMAS_LOGD(-1, "[ERASE_NODE_CALLBACK]", "Erasing agent %s from graph.", ID_C_STR(node->getId()));
		// Deletes task and agent
		model.getGroup(agent->groupId()).job().remove(*agent->task());
		delete agent->task();
		delete agent;
	}

	Model::Model(AgentGraph& graph, LoadBalancingAlgorithm& load_balancing)
		: gid(0), _graph(graph), _runtime(_scheduler), _loadBalancingJob(LB_JID), load_balancing_task(_graph, load_balancing, _scheduler, _runtime) {
			_loadBalancingJob.add(load_balancing_task);
			_graph.addCallOnInsertNode(&insert_node_callback);
			_graph.addCallOnEraseNode(&erase_node_callback);
		}

	Model::~Model() {
		//_graph.addOnInsert(new InsertNodeCallback(*this));
		for(auto group : _groups)
			delete group.second;
	}

	api::model::AgentGroup& Model::getGroup(GroupId id) {
		return *_groups.at(id);
	}

	api::model::AgentGroup& Model::buildGroup() {
		AgentGroup* group = new AgentGroup(gid, _graph, job_id++);
		_groups.insert({gid, group}); 
		gid++;
		return *group;
	}

	void LoadBalancingTask::run() {
		agent_graph.balance(load_balancing);
	}
}
