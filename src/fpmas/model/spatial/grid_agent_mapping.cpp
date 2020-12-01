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

}}
