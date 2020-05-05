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
		SYNC_MODE = GhostMode,
		typename... AgentTypes
		> class Grid
			: public Environment<S, Cell<S, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, Cell<S, AgentTypes...>, AgentTypes...> env_type;
					typedef Grid<S, AgentTypes...> grid_type;
					typedef Cell<S, AgentTypes...> cell_type;

				public:
					const int width;
					const int height;
					const int neighborhoodRange;

					const DistributedId id(unsigned int x, unsigned int y) {
						return {this->mpiCommunicator.getRank(), y * width + x};
					};
					Grid(int width, int height, Neighborhood<grid_type>* = new VonNeumann<grid_type>(1));

					template<typename AgentType, typename... Args> typename env_type::node_type* buildAgent(int x, int y, Args... args) {
						auto agentNode = this->buildNode(std::unique_ptr<typename grid_type::agent_type>(new AgentType(args...)));
						this->link(
								agentNode->getId(),
								id(x, y),
								LOCATION
								);
						return agentNode;
					}
			};

	template<SYNC_MODE, typename... AgentTypes>
		Grid<S, AgentTypes...>::Grid(int width, int height, Neighborhood<grid_type>* neighborhood)
			: width(width), height(height), neighborhoodRange(neighborhood->range) {
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
				for (int j = 0; j < height; j++) {
					for(int i = 0; i < width; i++) {
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
					neighborhood->linkNeighbors(*this, cell.first, cell.second);
				}
			}
			delete neighborhood;
		}

}
#endif
