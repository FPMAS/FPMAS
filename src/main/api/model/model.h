#include "api/graph/parallel/distributed_graph.h"
#include "api/scheduler/scheduler.h"
#include "api/runtime/runtime.h"

namespace FPMAS::api::model {
	class Agent;
	typedef api::graph::parallel::DistributedNode<std::unique_ptr<Agent*>> AgentNode;

	class Agent {
		public:
			virtual AgentNode* node() = 0;

			virtual void act() = 0;
	};

	class AgentGroup {
		public:
			virtual void add(Agent*) = 0;
			virtual api::scheduler::Job* job() const = 0;

			virtual const std::vector<Agent*>& agents() const = 0;
	};

	class Model {
		public:
			virtual api::graph::parallel::DistributedGraph<std::unique_ptr<Agent*>>& graph() = 0;

			virtual api::scheduler::Scheduler& scheduler() = 0;
			virtual api::scheduler::Job* loadBalancingJob() = 0;

			virtual AgentGroup* buildGroup() = 0;

			virtual api::runtime::Runtime& runtime() = 0;
	};
}
