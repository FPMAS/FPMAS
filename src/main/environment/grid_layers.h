#ifndef GRID_LAYERS_H
#define GRID_LAYERS_H

#include "utils/macros.h"

using FPMAS::graph::base::LayerId;

namespace FPMAS::environment::grid {
	static constexpr LayerId LOCATION = 0;
	static constexpr LayerId MOVABLE_TO = LOCATION + 1;
	static constexpr LayerId PERCEPTIONS = MOVABLE_TO + 1;
	static constexpr LayerId PERCEIVABLE_FROM = PERCEPTIONS +1;
	static constexpr LayerId NEIGHBOR_CELL(int d) {
		return PERCEIVABLE_FROM + d - 1;
	}
}
#endif
