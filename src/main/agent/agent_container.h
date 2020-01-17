#ifndef AGENT_CONTAINER_H
#define AGENT_CONTAINER_H

#include "utils/simulation_parameters.h"
#include "zoltan_cpp.h"
#include "agent/agent_base.h"
#include "environment/graph/base/graph.h"

namespace FPMAS {
	using graph::Graph;
	namespace agent {

		class AgentContainer {

			private:
				Graph<AgentBase*> g;

			public:
				AgentContainer(SimulationParameters*, Zoltan*);

		};
	}
}
#endif
