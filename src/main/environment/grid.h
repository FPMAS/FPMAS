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

	static constexpr int neighborLayer(int userLayers, int d) {
		return userLayers+d-1;
	}

	static constexpr int layerCount(int userLayers, int neighborhoodRange) {
		return
			userLayers+ // User layers
			neighborhoodRange+ // Cell neighborhood layers count
			1+ // Location layer
			1+ // movableTo layer
			1+ // Perception layer
			1; // perceivableFrom layer
	}

	static constexpr LayerId locationLayer(int userLayers, int neighborhoodRange) {
		return userLayers + neighborhoodRange;
	}

	static constexpr LayerId movableTo(int userLayers, int neighborhoodRange) {
		return locationLayer(userLayers, neighborhoodRange) + 1;
	}

	static constexpr LayerId perceptionsLayer(int userLayers, int neighborhoodRange) {
		return movableTo(userLayers, neighborhoodRange) + 1;
	}

	static constexpr LayerId perceivableFromLayer(int userLayers, int neighborhoodRange) {
		return perceptionsLayer(userLayers, neighborhoodRange) + 1;
	}

	template<
		template<typename, typename, int> class Neighborhood = VonNeumann,
		int Range = 1,
		SYNC_MODE = GhostMode,
		int N = 1, // User provided layers
		typename... AgentTypes> class Grid
			: public Environment<S, layerCount(N, Range), Cell<Range, S, layerCount(N, Range), AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, layerCount(N, Range), Cell<Range, S, layerCount(N, Range), AgentTypes...>, AgentTypes...> env_type;
					typedef Grid<Neighborhood, Range, S, N, AgentTypes...> grid_type;
					typedef Cell<Range, S, layerCount(N, Range), AgentTypes...> cell_type;
					static const int userLayers = N;

				private:
					const int _width;
					const int _height;
					Neighborhood<grid_type, cell_type, Range> neighborhood;

				public:

					const DistributedId id(unsigned int x, unsigned int y) {
						return {this->mpiCommunicator.getRank(), y * _width + x};
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
