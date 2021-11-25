#include "test_agents.h"

namespace fpmas { namespace model {
	bool operator==(const GridCell&, const GridCell&) {
		return true;
	}
	bool operator==(const GraphCell&, const GraphCell&) {
		return true;
	}
}}

namespace model { namespace test {
	bool operator==(const SpatialAgent&, const SpatialAgent&) {
		return true;
	}
	bool operator==(const GridAgent&, const GridAgent&) {
		return true;
	}
}}

