#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

namespace FPMAS::agent {
	template<SYNC_MODE, typename... Types> class Agent;
}

using FPMAS::agent::Agent;

namespace FPMAS::environment {

	template<SYNC_MODE, typename... AgentTypes>
	class Environment
	: public DistributedGraph<std::unique_ptr<Agent<S, AgentTypes...>>, S> {
		public:
			typedef typename DistributedGraph<std::unique_ptr<Agent<S, AgentTypes...>>, S>::node_type node_type;
			typedef typename DistributedGraph<std::unique_ptr<Agent<S, AgentTypes...>>, S>::node_ptr node_ptr;
			typedef Agent<S, AgentTypes...> agent_type;
			void step();

	};
}
#endif
