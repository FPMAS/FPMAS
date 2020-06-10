#ifndef MODEL_API_H
#define MODEL_API_H

#include "api/graph/parallel/distributed_graph.h"
#include "api/scheduler/scheduler.h"
#include "api/runtime/runtime.h"

namespace FPMAS::api::model {
	class Agent;
	typedef api::graph::parallel::DistributedNode<Agent*> AgentNode;

	class Agent {
		public:
			virtual AgentNode* node() = 0;
			virtual const AgentNode* node() const = 0;
			virtual void setNode(AgentNode*) = 0;

			virtual void act() = 0;

			virtual ~Agent(){}
	};

	class AgentTask : public api::scheduler::NodeTask<Agent*> {
		public:
			virtual const Agent& agent() const = 0;
			virtual ~AgentTask(){}
	};

	class AgentGroup {
		public:
			virtual void add(Agent*) = 0;
			virtual const api::scheduler::Job& job() const = 0;

			virtual const std::vector<Agent*>& agents() const = 0;
			virtual ~AgentGroup(){}
	};

	class Model {
		public:
			typedef api::graph::parallel::DistributedGraph<Agent*> AgentGraph;

			virtual AgentGraph& graph() = 0;
			virtual api::scheduler::Scheduler& scheduler() = 0;
			virtual api::runtime::Runtime& runtime() = 0;

			virtual const api::scheduler::Job& loadBalancingJob() const = 0;

			virtual AgentGroup* buildGroup() = 0;
			virtual const std::vector<AgentGroup*>& groups() const = 0;
			virtual ~Model(){}
	};
}
#endif
