#ifndef FPMAS_GRID_AGENT_MAPPING_G
#define FPMAS_GRID_AGENT_MAPPING_G

#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/random/distribution.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	class RandomGridAgentMapping : public api::model::SpatialAgentMapping<api::model::GridCell> {
		private:
			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>> count_map;
		public:
			RandomGridAgentMapping(
					api::random::Distribution<DiscreteCoordinate>&& x,
					api::random::Distribution<DiscreteCoordinate>&& y,
					std::size_t agent_count);

			RandomGridAgentMapping(
					api::random::Distribution<DiscreteCoordinate>& x,
					api::random::Distribution<DiscreteCoordinate>& y,
					std::size_t agent_count);

			std::size_t countAt(api::model::GridCell* cell) override {
				// When values are not in the map, 0 (default initialized) will
				// be returned
				return count_map[cell->location().x][cell->location().y];
			}
	};

	class UniformGridAgentMapping : public RandomGridAgentMapping {
		public:
			UniformGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count) 
				: RandomGridAgentMapping(
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_width-1),
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_height-1),
						agent_count) {}
	};

}}
#endif
