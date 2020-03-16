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
		static constexpr int GridLayer = 1;
		class Cell : public Agent<Cell> {
			private:
				const int _x;
				const int _y;
				void act() override {};
			public:
				Cell() : _x(0), _y(0) {}
				Cell(int x, int y) : _x(x), _y(y) {}

				Cell(const Cell& other) : Cell(other._x, other._y) {}

				const int x() const {
					return _x;
				}
				const int y() const {
					return _y;
				}
		};

		// T = agent
		template<int W, int H, SYNC_MODE = GhostMode, int N = 1> class Grid : public Environment<S, N+1, Cell> {
			public:
				static constexpr NodeId id(int x, int y) {
					return y * W + x;
				};
				Grid();

		};

		template<int W, int H, SYNC_MODE, int N> Grid<W, H, S, N>::Grid() {
			for(int i = 0; i < W; i++) {
				for (int j = 0; j < H; j++) {
					// Build cell
					this->buildNode(
						id(i, j),
						std::unique_ptr<Agent<Cell>>(new Cell(i, j))
						);
				}
			}
			int arcId = 0;
			for(auto node : this->getNodes()) {
				const Cell& cell = dynamic_cast<Cell&>(*node.second->data()->read());
				if(cell.x() > 0) {
					this->link(
							node.first,
							id(cell.x() - 1, cell.y()),
							arcId++,
							GridLayer
							);
				}
				if(cell.x() < W - 1) {
					this->link(
							node.first,
							id(cell.x() + 1, cell.y()),
							arcId++,
							GridLayer
							);
				}
				if(cell.y() > 0) {
					this->link(
							node.first,
							id(cell.x(), cell.y() - 1),
							arcId++,
							GridLayer
							);
				}
				if(cell.y() < H - 1) {
					this->link(
							node.first,
							id(cell.x(), cell.y() + 1),
							arcId++,
							GridLayer
							);
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
				j["x"] = value.x();
				j["y"] = value.y();
			}

			static Cell from_json(const json& j) {
				// same thing, but with the "from_json" method
				return Cell(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
