#ifndef FAKE_GRID_AGENT_H
#define FAKE_GRID_AGENT_H

#include "environment/grid_agent.h"
#include "environment/grid.h"

class FakeGridAgent : public GridAgent<GhostMode, FakeGridAgent> {
	public:
		FakeGridAgent() : GridAgent<GhostMode, FakeGridAgent>(1, 1) {}
		void act(node_ptr, env_type&) override {}
};

namespace nlohmann {
	template<>
    struct adl_serializer<FakeGridAgent> {
		static void to_json(json& j, const FakeGridAgent& data) {
			nlohmann::adl_serializer<GridAgent<GhostMode, FakeGridAgent>>
				::to_json(j, data);
		}

		static FakeGridAgent from_json(const json& j) {
			return FakeGridAgent();
		}

	};
}
#endif
