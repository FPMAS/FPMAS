#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "api/graph/parallel/synchro/sync_mode.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename NodeType, typename ArcType>
		class HardDataSync {
			public:
				void synchronize(
						FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
						);
		};
}

#endif
