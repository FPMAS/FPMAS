#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

namespace FPMAS::agent {
	template<SYNC_MODE, int N, typename... Types> class Agent;
}

using FPMAS::agent::Agent;

namespace FPMAS::environment {

	template<SYNC_MODE, int N, typename... AgentTypes>
	class Environment
	: public DistributedGraph<std::unique_ptr<Agent<S, N, AgentTypes...>>, S, N> {
		public:
			typedef typename DistributedGraph<std::unique_ptr<Agent<S, N, AgentTypes...>>, S, N>::node_ptr node_ptr;
			typedef Agent<S, N, AgentTypes...> agent_type;
			void step();

	};
}
#endif
