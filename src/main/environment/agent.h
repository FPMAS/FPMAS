#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "grid.h"

namespace FPMAS::environment::grid {

	template<
		int Range,
		SYNC_MODE,
		int N,
		typename... AgentTypes
	> class GridAgent : public Agent<S, layerCount(N, Range), Cell<S, layerCount(N, Range), AgentTypes...>, AgentTypes...> {


	};
}

#endif
