#include "grid_agent_mapping.h"

#include "fpmas/random/random.h"

namespace fpmas { namespace model {
	RandomGridAgentMapping::RandomGridAgentMapping(
			api::random::Distribution<DiscreteCoordinate>&& x,
			api::random::Distribution<DiscreteCoordinate>&& y,
			std::size_t agent_count)
	: RandomGridAgentMapping(x, y, agent_count) {
	}

	RandomGridAgentMapping::RandomGridAgentMapping(
			api::random::Distribution<DiscreteCoordinate>& x,
			api::random::Distribution<DiscreteCoordinate>& y,
			std::size_t agent_count) {
		random::mt19937_64 rd;

		for(std::size_t i = 0; i < agent_count; i++) {
			DiscretePoint p {x(rd), y(rd)};
			count_map[p.x][p.y]++;
		}
	}

	ConstrainedGridAgentMapping::ConstrainedGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count,
					std::size_t max_agent_by_cell) {
		if(grid_width * grid_height * max_agent_by_cell < agent_count)
			return;
		random::mt19937_64 rd;
		std::size_t current_agent_count = agent_count;
		std::size_t n = 0;
		while(current_agent_count > 0 && n < max_agent_by_cell) {
			std::size_t n_agent;
			if(n == max_agent_by_cell-1)
				// Last iteration, dispach all remaining agents
				n_agent = current_agent_count;
			else
				n_agent = std::max(1ul, agent_count / max_agent_by_cell);

			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>>
				local_map;
			for(std::size_t i = 0; i < n_agent; i++) {
				DiscreteCoordinate x = i % grid_width;
				DiscreteCoordinate y = i / grid_width;
				assert((std::size_t) (x + grid_width * y) == i);
				local_map[x][y] = 1;
			}
			std::size_t n_cell = grid_width * grid_height;
			for(std::size_t i = 0; i < n_cell-1; i++) {
				std::size_t j = i+random::UniformIntDistribution<std::size_t>(0, n_cell-i)(rd);
				DiscreteCoordinate x_i = i % grid_width;
				DiscreteCoordinate y_i = i / grid_width;
				DiscreteCoordinate x_j = j % grid_width;
				DiscreteCoordinate y_j = j / grid_width;
				// Swap
				std::size_t old_i = local_map[x_i][y_i];
				local_map[x_i][y_i] = local_map[x_j][y_j];
				local_map[x_j][y_j] = old_i;
			}
			// Merge maps
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
