#ifndef FPMAS_GRID_AGENT_MAPPING_G
#define FPMAS_GRID_AGENT_MAPPING_G

/**\file src/fpmas/model/spatial/grid_agent_mapping.h
 * Grid agent mappings implementation.
 */

#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/random/distribution.h"
#include "fpmas/random/random.h"
#include "spatial_agent_mapping.h"


namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * api::model::SpatialAgentMapping implementation for grid environments.
	 *
	 * Agents are distributed on the environment according to random
	 * distributions.
	 *
	 * The RandomMapping::rd random generator is used, and can be seeded with
	 * RandomMapping::seed() or fpmas::seed().
	 */
	class RandomGridAgentMapping : public api::model::SpatialAgentMapping<api::model::GridCell> {
		private:
			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>> count_map;
		public:
			/**
			 * RandomGridAgentMapping constructor
			 *
			 * The x/y position of `agent_count` agents is computed using the
			 * provided random distributions. The same default deterministic random number
			 * generator is used on all processes, to ensure that all the
			 * processes compute the same agent mapping. To ensure this, it is
			 * also required that x/y distributions are the same on all
			 * processes.
			 *
			 * Once the x/y position of all agents have been computed, the
			 * number of agents on each x/y position is stored and used to
			 * produce results returned by the countAt() method.
			 *
			 * @tparam DistributionX x coordinates distribution
			 * @tparam DistributionY y coordinates distribution
			 *
			 * @param x x distribution
			 * @param y y distribution
			 * @param agent_count agent count
			 */
			template<typename DistributionX, typename DistributionY>
			RandomGridAgentMapping(
					DistributionX&& x,
					DistributionY&& y,
					std::size_t agent_count
					);

			std::size_t countAt(api::model::GridCell* cell) override {
				// When values are not in the map, 0 (default initialized) will
				// be returned
				return count_map[cell->location().x][cell->location().y];
			}
	};

	template<typename DistributionX, typename DistributionY>
		RandomGridAgentMapping::RandomGridAgentMapping(
				DistributionX&& x,
				DistributionY&& y,
				std::size_t agent_count) {

			for(std::size_t i = 0; i < agent_count; i++) {
				DiscretePoint p {x(RandomMapping::rd), y(RandomMapping::rd)};
				count_map[p.x][p.y]++;
			}
		}


	/**
	 * api::model::SpatialAgentMapping implementation for grid environments.
	 *
	 * Agents are uniformly distributed on a grid of the specified size.
	 */
	class UniformGridAgentMapping : public RandomGridAgentMapping {
		public:
			/**
			 * UniformGridAgentMapping constructor.
			 *
			 * X and Y coordinates are uniformly assigned in [0, grid_width-1],
			 * and [0, grid_height-1] respectively.
			 *
			 * @param grid_width grid width
			 * @param grid_height grid height
			 * @param agent_count agent count
			 */
			UniformGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count
					) 
				: RandomGridAgentMapping(
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_width-1),
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_height-1),
						agent_count) {}
	};

	/**
	 * api::model::SpatialAgentMapping implementation for grid environments.
	 *
	 * The ConstrainedGridAgentMapping allows to uniformly distribute agents
	 * over a grid, while specifying a maximum count of agents by cell.
	 *
	 * The RandomMapping::rd random generator is used, and can be seeded with
	 * RandomMapping::seed() or fpmas::seed().
	 */
	class ConstrainedGridAgentMapping : public api::model::SpatialAgentMapping<api::model::GridCell> {
		private:
			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>>
				count_map;
		public:
			/**
			 * ConstrainedGridAgentMapping constructor.
			 *
			 * `agent_count` agents will be uniformly distributed on the grid
			 * defined by `(grid_width, grid_height)`, considering the origin
			 * at `(0, 0)`, with `max_agent_by_cell` as the maximum capacity of
			 * each cell.
			 *
			 * @param grid_width Grid width
			 * @param grid_height Grid height
			 * @param agent_count total count of agents to map
			 * @param max_agent_by_cell maximum count of agents allowed on each
			 * cell
			 */
			ConstrainedGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count,
					std::size_t max_agent_by_cell
					);

		std::size_t countAt(api::model::GridCell* cell) override;
	};

}}
#endif
