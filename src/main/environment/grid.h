#ifndef GRID_H
#define GRID_H

#include "graph/parallel/distributed_graph.h"
#include "cell.h"
#include "agent/agent.h"
#include "neigborhood.h"

using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::agent::Agent;

#define DEFINE_NEIGHBOR_LAYER(i, N) \
	static constexpr int Neighbor_##i N+i;
	

namespace FPMAS::environment {

	template<SYNC_MODE, int N, typename... AgentTypes>
	class Environment
	: public DistributedGraph<std::unique_ptr<Agent<AgentTypes...>>, S, N> {
		public:
			void step();

	};

	namespace grid {
		// T = agent
		template<
			template<typename, int> class Neighborhood = VonNeumann,
			int Range = 1,
			SYNC_MODE = GhostMode,
			int N = 1> class Grid : public Environment<S, N+Range, Cell> {
			private:
				const int _width;
				const int _height;
				Neighborhood<Grid<Neighborhood, Range, S, N>, Range> neighborhood;
			public:
				static constexpr int Neighbor_Layer(int d) {
					return N+d-1;
				}

				constexpr NodeId id(int x, int y) {
					return y * _width + x;
				};
				Grid(int width, int height);

				const int width() const {
					return _width;
				}
				const int height() const {
					return _height;
				}

		};

		template<template<typename, int> class Neighborhood, int Range, SYNC_MODE, int N>
			Grid<Neighborhood, Range, S, N>::Grid(int width, int height): _width(width), _height(height), neighborhood(*this) {
			for(int i = 0; i < _width; i++) {
				for (int j = 0; j < _height; j++) {
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
				neighborhood.linkNeighbors(node.first, cell);
				/*
				 *if(cell.x() > 0) {
				 *    this->link(
				 *            node.first,
				 *            id(cell.x() - 1, cell.y()),
				 *            arcId++,
				 *            GridLayer
				 *            );
				 *}
				 *if(cell.x() < W - 1) {
				 *    this->link(
				 *            node.first,
				 *            id(cell.x() + 1, cell.y()),
				 *            arcId++,
				 *            GridLayer
				 *            );
				 *}
				 *if(cell.y() > 0) {
				 *    this->link(
				 *            node.first,
				 *            id(cell.x(), cell.y() - 1),
				 *            arcId++,
				 *            GridLayer
				 *            );
				 *}
				 *if(cell.y() < H - 1) {
				 *    this->link(
				 *            node.first,
				 *            id(cell.x(), cell.y() + 1),
				 *            arcId++,
				 *            GridLayer
				 *            );
				 *}
				 */
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
