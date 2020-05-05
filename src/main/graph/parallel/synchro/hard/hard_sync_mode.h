#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "api/graph/parallel/synchro/sync_mode.h"
#include "hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename NodeType, typename ArcType>
		class HardDataSync {
			public:
				void synchronize(
						FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&
						);
		};

	template<typename T>
		class HardSyncMode : public FPMAS::api::graph::parallel::synchro::SyncMode<T, HardSyncMutex, HardSyncLinker, HardSyncData> {

		};
}

#endif
