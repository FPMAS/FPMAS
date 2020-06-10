#include "model.h"
#include "graph/parallel/distributed_node.h"

namespace FPMAS::model {

	const JID Model::LB_JID = 0;

	AgentGroup::AgentGroup(AgentGraph& agent_graph, JID job_id)
		: agent_graph(agent_graph), _job(job_id), sync_graph_task(agent_graph) {
			_job.setEndTask(sync_graph_task);
	}

	void AgentGroup::add(api::model::Agent* agent) {
		_agents.push_back(agent);
		AgentTask*& task = agent_tasks.emplace_back(new AgentTask(*agent));
		auto node = agent_graph.buildNode(agent);
		agent->setNode(node);
		_job.add(*task);
	}

	AgentGroup::~AgentGroup() {
		for(auto* agent : _agents)
			delete agent;
		for(auto* agent_task : agent_tasks)
			delete agent_task;
	}

	Model::Model(AgentGraph& graph, LoadBalancingAlgorithm& load_balancing)
		: _graph(graph), _runtime(_scheduler), _loadBalancingJob(LB_JID), load_balancing_task(_graph, load_balancing, _scheduler, _runtime) {
			_loadBalancingJob.add(load_balancing_task);
		}

	Model::~Model() {
		for(auto group : _groups)
			delete group;
	}

	AgentGroup* Model::buildGroup() {
		AgentGroup* group = new AgentGroup(_graph, job_id++);
		_groups.push_back(group); 
		return group;
	}

	void LoadBalancingTask::run() {
		agent_graph.balance(load_balancing);
	};
}
