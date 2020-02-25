#ifndef GRID_H
#define GRID_H


#include "graph/parallel/distributed_graph.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::synchro::GhostData;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::agent::Agent;

namespace FPMAS::environment {
	enum GridLayer {
		GRID = 0,
		DEFAULT = 1
	};

	template<SYNC_MODE, typename LayerType, int N, typename AgentTypeEnum, typename... AgentTypes> class Environment
		: public DistributedGraph<std::unique_ptr<Agent<AgentTypeEnum, AgentTypes...>>, S, LayerType, N> {
		public:
			void step();

	};

	namespace grid {
		enum CellType {
			CELL
		};

		class Cell : public Agent<CellType, Cell> {
			private:
				void act() override {};

			public:
				static const CellType type = CELL;

				Cell() : Agent<CellType, Cell>(CELL) {};

		};
/*
 *
 *        class CellSerializer {
 *            public:
 *            static void to_json(json& j, const Agent<CellType, CellSerializer>& agent) {
 *                switch(agent.getType()) {
 *                    case Cell:
 *                        to_json(j, dynamic_cast<const class Cell&>(agent));
 *                        break;
 *                }
 *            }
 *
 *            static void from_json(const json& j, Agent<CellType, CellSerializer>& agent, CellType type) {
 *
 *            }
 *
 *        };
 */
		// T = agent
		template<SYNC_MODE = GhostData, typename LayerType = GridLayer, int N = 2> class Grid : public Environment<S, LayerType, N, CellType, Cell> {
			public:
				Grid(int width, int height);

		};

		template<SYNC_MODE, typename LayerType, int N> Grid<S, LayerType, N>::Grid(int width, int height) {
			int id=0;
			for(int i = 0; i < width; i++) {
				for (int j = 0; j < height; j++) {
					// Build cell
					this->buildNode(id++, std::unique_ptr<Agent<CellType, Cell>>(new Cell()));
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
