#ifndef MODEL_H
#define MODEL_H
#include "api/model/model.h"
#include "api/graph/parallel/distributed_graph.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"

namespace FPMAS::model {

	class AgentTask : public api::model::AgentTask {
		private:
			api::model::Agent& _agent;
		public:
			AgentTask(api::model::Agent& agent)
				: _agent(agent) {}

			const api::model::Agent& agent() const override {return _agent;}

			void run() override {
				_agent.act();
			}
	};

	class SynchronizeGraphTask : public api::scheduler::Task {
		public:
			typedef typename api::model::Model::AgentGraph AgentGraph;
		private:
			AgentGraph& agent_graph;
		public:
			SynchronizeGraphTask(AgentGraph& agent_graph)
				: agent_graph(agent_graph) {}

			void run() override {
				agent_graph.synchronize();
			}
	};

	class AgentGroup : public api::model::AgentGroup {
		public:
			typedef typename api::model::Model::AgentGraph AgentGraph;
		private:
			AgentGraph& agent_graph;
			scheduler::Job _job;
			SynchronizeGraphTask sync_graph_task;
			std::vector<api::model::Agent*> _agents;

		public:
			AgentGroup(AgentGraph& agent_graph, JID job_id);

			const scheduler::Job& job() const override {return _job;}

			const std::vector<api::model::Agent*>& agents() const override {return _agents;}
			void add(api::model::Agent*) override;
	};

	class LoadBalancingTask : public api::scheduler::Task {
		public:
			typedef typename api::model::Model::AgentGraph AgentGraph;
			typedef api::graph::parallel::LoadBalancing<std::unique_ptr<api::model::Agent*>>
				LoadBalancingAlgorithm;
			typedef typename LoadBalancingAlgorithm::ConstNodeMap ConstNodeMap;
			typedef typename LoadBalancingAlgorithm::PartitionMap PartitionMap;

		private:
			AgentGraph& agent_graph;
			LoadBalancingAlgorithm& load_balancing;
			api::scheduler::Scheduler& scheduler;
			api::runtime::Runtime& runtime;

		public:
			LoadBalancingTask(
					AgentGraph& agent_graph,
					LoadBalancingAlgorithm& load_balancing,
					api::scheduler::Scheduler& scheduler,
					api::runtime::Runtime& runtime
					) : agent_graph(agent_graph), load_balancing(load_balancing), scheduler(scheduler), runtime(runtime) {}

			void run() override;
	};

/*
 *    class LoadBalancingJob : public scheduler::Job {
 *        private:
 *            api::scheduler::Scheduler& scheduler;
 *            api::runtime::Runtime& runtime;
 *
 *        public:
 *            LoadBalancingJob(
 *                JID id, api::scheduler::Scheduler& scheduler, api::runtime::Runtime& runtime
 *                ) : scheduler::Job(id), scheduler(scheduler), runtime(runtime) {}
 *
 *    };
 */

	class Model : public api::model::Model {
		public:
			typedef typename LoadBalancingTask::LoadBalancingAlgorithm LoadBalancingAlgorithm;
		private:
			AgentGraph& _graph;
			scheduler::Scheduler _scheduler;
			runtime::Runtime _runtime;
			scheduler::Job _loadBalancingJob;
			LoadBalancingTask load_balancing_task;

			std::vector<api::model::AgentGroup*> _groups;
			
			JID job_id = LB_JID + 1;
			

		public:
			static const JID LB_JID;
			Model(AgentGraph& graph, LoadBalancingAlgorithm& load_balancing);

			AgentGraph& graph() override {return _graph;}
			scheduler::Scheduler& scheduler() override {return _scheduler;}
			runtime::Runtime& runtime() override {return _runtime;}

			const scheduler::Job& loadBalancingJob() const override {return _loadBalancingJob;}

			AgentGroup* buildGroup() override;
			const std::vector<api::model::AgentGroup*>& groups() const override {return _groups;}

			~Model();
	};
};
#endif
