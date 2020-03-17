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
	: public DistributedGraph<std::unique_ptr<Agent<N, AgentTypes...>>, S, N> {
		public:
			typedef Agent<N, AgentTypes...> agent_type;
		public:
			void step();

	};

	namespace grid {
		// T = agent
		template<
			template<typename, typename, int> class Neighborhood = VonNeumann,
			int Range = 1,
			SYNC_MODE = GhostMode,
			int N = 1, // User provided layers
			typename... AgentTypes> class Grid : public Environment<S, N+Range, Cell<N, Range, AgentTypes...>, AgentTypes...> {
			public:
				typedef Environment<S, N+Range, Cell<N, Range, AgentTypes...>, AgentTypes...> env_type;
				typedef Grid<Neighborhood, Range, S, N, AgentTypes...> grid_type;
				typedef Cell<N, Range, AgentTypes...> cell_type;

			private:

				const int _width;
				const int _height;
				Neighborhood<grid_type, cell_type, Range> neighborhood;
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

		template<template<typename, typename, int> class Neighborhood, int Range, SYNC_MODE, int N, typename... AgentTypes>
			Grid<Neighborhood, Range, S, N, AgentTypes...>::Grid(int width, int height): _width(width), _height(height), neighborhood(*this) {
			for(int i = 0; i < _width; i++) {
				for (int j = 0; j < _height; j++) {
					// Build cell
					this->buildNode(
						id(i, j),
						std::unique_ptr<typename env_type::agent_type>(new cell_type(i, j))
						);
				}
			}
			int arcId = 0;
			for(auto node : this->getNodes()) {
				const cell_type& cell = dynamic_cast<cell_type&>(*node.second->data()->read());
				neighborhood.linkNeighbors(node.first, cell);
			}
		}

	}
}

namespace nlohmann {
	using FPMAS::environment::grid::Cell;
	template <int N, int Range, typename... AgentTypes>
		struct adl_serializer<Cell<N, Range, AgentTypes...>> {
			static void to_json(json& j, const Cell<N, Range, AgentTypes...>& value) {
				// calls the "to_json" method in T's namespace
				j["x"] = value.x();
				j["y"] = value.y();
			}

			static Cell<N, Range, AgentTypes...> from_json(const json& j) {
				// same thing, but with the "from_json" method
				return Cell<N, Range, AgentTypes...>(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
