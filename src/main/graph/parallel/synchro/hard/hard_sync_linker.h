#ifndef HARD_SYNC_LINKER_H
#define HARD_SYNC_LINKER_H

#include "api/graph/parallel/synchro/sync_mode.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename Arc>
	class HardSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<Arc> {
		public:
			void link(const Arc*) override {}
			void unlink(const Arc*) override {}
			void synchronize() override {}

	};
}

#endif
