#include "grid_agent_mapping.h"

namespace fpmas { namespace model {


	ConstrainedGridAgentMapping::ConstrainedGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count,
					std::size_t max_agent_by_cell) {
		if(grid_width * grid_height * max_agent_by_cell < agent_count)
			return;
		std::size_t current_agent_count = agent_count;
		std::size_t n = 0;
		while(current_agent_count > 0 && n < max_agent_by_cell) {
			std::size_t n_agent;
			if(n == max_agent_by_cell-1)
				// Last iteration, dispach all remaining agents
				n_agent = current_agent_count;
			else
				// Randomly distribute those agents in the current iteration
				n_agent = std::max(1ul, agent_count / max_agent_by_cell);

			// Distribution of agent for this iteration.
			// At the end of the process, local_map is such as the value of its
			// keys is either 0 or 1, and the sum of all the keys is equal to
			// n_agent. In other terms, a unique random location is unirformly
			// assigned to n_agent.
			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>>
				local_map;
			// Initializes the agents at the beginning of the grid.
			for(std::size_t i = 0; i < n_agent; i++) {
				DiscreteCoordinate x = i % grid_width;
				DiscreteCoordinate y = i / grid_width;
				local_map[x][y] = 1;
			}
			std::size_t n_cell = grid_width * grid_height;
			for(std::size_t i = 0; i < n_cell-1; i++) {
				// Shuffles agents on the global grid (Fisher-Yates algorithm)
				std::size_t j = i+random::UniformIntDistribution<std::size_t>(0, n_cell-i)(RandomMapping::rd);
				DiscreteCoordinate x_i = i % grid_width;
				DiscreteCoordinate y_i = i / grid_width;
				DiscreteCoordinate x_j = j % grid_width;
				DiscreteCoordinate y_j = j / grid_width;
				// Swap
				std::size_t old_i = local_map[x_i][y_i];
				local_map[x_i][y_i] = local_map[x_j][y_j];
				local_map[x_j][y_j] = old_i;
			}
			// The content of the local is added to the global count_map.
			// In consequence, each cell can contain more that one agent in
			// count_map, but no more than max_agent_by_cell (since this is the
			// maximum count of iterations, i.e. the maximum count of local_map
			// built).
			for(auto x : local_map)
				for(auto y : x.second)
					count_map[x.first][y.first] += y.second;
			n++;
			current_agent_count -= n_agent;
		}
		assert(current_agent_count == 0);
	}

	std::size_t ConstrainedGridAgentMapping::countAt(api::model::GridCell *cell) {
		return count_map[cell->location().x][cell->location().y];
	}
}}
