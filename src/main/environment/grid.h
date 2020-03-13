#ifndef GRID_H
#define GRID_H


#include "graph/parallel/distributed_graph.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::agent::Agent;

namespace FPMAS::environment {
	enum GridLayer {
		DEFAULT = 0,
		GRID = 1
	};

	template<SYNC_MODE, int N, typename... AgentTypes>
	class Environment
	: public DistributedGraph<std::unique_ptr<Agent<AgentTypes...>>, S, N> {
		public:
			void step();

	};

	namespace grid {
		class Cell : public Agent<Cell> {
			private:
				void act() override {};

		};

		// T = agent
		template<SYNC_MODE = GhostMode, int N = 1> class Grid : public Environment<S, N+1, Cell> {
			public:
				Grid(int width, int height);

		};

		template<SYNC_MODE, int N> Grid<S, N>::Grid(int width, int height) {
			int id=0;
			for(int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					// Build cell
					this->buildNode(id++, std::move(std::unique_ptr<Agent<Cell>>(new Cell())));
				}
			}
		}

	}
}

namespace nlohmann {
	using FPMAS::environment::grid::Cell;
	template <>
		struct adl_serializer<Cell> {
			static void to_json(json& j, const Cell& value) {
				// calls the "to_json" method in T's namespace
			}

			static void from_json(const json& j, Cell& value) {
				// same thing, but with the "from_json" method
			}
		};
}
#endif
