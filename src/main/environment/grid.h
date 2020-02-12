#ifndef GRID_H
#define GRID_H

#include "graph/parallel/distributed_graph.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::agent::Agent;

namespace FPMAS::environment {
	template<class T, template<class> class S> class Environment : public DistributedGraph<T, S> {
		public:
			void step();

	};

	namespace Grid {

		template<class T> class Cell : public Agent {
			private:
				T agent;
				void act() override;

		};
	
		// T = agent
		template<class T, template<class> class S> class Grid : public DistributedGraph<T, S> {
			public:
				Grid(int width, int height);

		};

	}
}
#endif
