#ifndef GRID_H
#define GRID_H


#include "graph/parallel/distributed_graph.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::synchro::GhostData;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::agent::Agent;
using FPMAS::agent::AgentPtr;

namespace FPMAS::environment {
	enum GridLayer {
		GRID = 0,
		DEFAULT = 1
	};

	template<class T, SYNC_MODE, typename LayerType, int N> class Environment : public DistributedGraph<T, S, LayerType, N> {
		public:
			void step();

	};

	namespace grid {

		class Cell : public Agent {
			private:
				void act() override {};

		};
	
		// T = agent
		template<SYNC_MODE = GhostData, typename LayerType = GridLayer, int N = 2> class Grid : public Environment<AgentPtr, S, LayerType, N> {
			public:
				Grid(int width, int height);

		};

		template<SYNC_MODE, typename LayerType, int N> Grid<S, LayerType, N>::Grid(int width, int height) {
			int id=0;
			for(int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					// Build cell
					this->buildNode(id++, AgentPtr(new Cell()));
				}
			}
		}

	}
}
#endif
