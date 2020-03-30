#ifndef GRID_H
#define GRID_H

#include "cell.h"
#include "environment.h"
#include "agent/agent.h"
#include "neigborhood.h"

using FPMAS::graph::base::LayerId;
using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {

	template<
		template<typename> class Neighborhood = VonNeumann,
		SYNC_MODE = GhostMode,
		typename... AgentTypes> class Grid
			: public Environment<S, Cell<S, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, Cell<S, AgentTypes...>, AgentTypes...> env_type;
					typedef Grid<Neighborhood, S, AgentTypes...> grid_type;
					typedef Cell<S, AgentTypes...> cell_type;

				private:
					const int _width;
					const int _height;
					Neighborhood<grid_type> neighborhood;

				public:

					const DistributedId id(unsigned int x, unsigned int y) {
						return {this->mpiCommunicator.getRank(), y * _width + x};
					};
					Grid(int width, int height, unsigned int neighborhoodRange = 1);

					const int width() const {
						return _width;
					}
					const int height() const {
						return _height;
					}

			};

	template<template<typename> class Neighborhood, SYNC_MODE, typename... AgentTypes>
		Grid<Neighborhood, S, AgentTypes...>::Grid(int width, int height, unsigned int neighborhoodRange)
			: _width(width), _height(height), neighborhood(*this, neighborhoodRange) {
			// TODO: replace this hack with a true distributed build
			if(this->getMpiCommunicator().getRank() == 0) {
				/*
				 * For a 3x2 grid for ex, nodes are built in this order (node ids
				 * start from 0) :
				 * - j = 0 : 0 -> 1 -> 2
				 * - j = 1 : 3 -> 4 -> 5
				 * From there can be deduced the id(x, y) = y*width+x function.
				 */
				std::unordered_map<DistributedId, const cell_type*> cells;
				for (int j = 0; j < _height; j++) {
					for(int i = 0; i < _width; i++) {
						// Build cell
						cell_type* cell = new cell_type(i, j);
						auto node = this->buildNode(
								std::unique_ptr<typename env_type::agent_type>(cell),
								1
								);
						cells[node->getId()] = cell;
					}
				}
				for(auto cell : cells) {
					neighborhood.linkNeighbors(cell.first, cell.second);
				}
			}
		}

}
#endif
