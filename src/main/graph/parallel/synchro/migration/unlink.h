#ifndef UNLINK_MIGRATION_H
#define UNLINK_MIGRATION_H

#include "communication/communication.h"
#include "communication/migrate.h"
#include "../../distributed_graph_base.h"

namespace FPMAS::graph::parallel {
	template<typename T, SYNC_MODE> class GhostArc;
	namespace synchro {
		namespace modes {
			template<typename> class GhostMode;
		}
		using modes::GhostMode;
		template<typename T> class UnlinkPostMigrate
			: public FPMAS::communication::PostMigrate<DistributedId, DistributedGraphBase<T,GhostMode>> {
				public:
					void operator()(
							DistributedGraphBase<T,GhostMode>& dg, 
							std::unordered_map<int, std::vector<DistributedId>> exportMap,
							std::unordered_map<int, std::vector<DistributedId>> importMap
							) override {
						for(auto item : importMap) {
							for(DistributedId arcId : item.second) {
								FPMAS_LOGD(
									dg.getMpiCommunicator().getRank(),
									"UNLINK_MIGRATE", "Imported unlink : %s",
									ID_C_STR(arcId)
									);
								try {
									auto arc = dg.getGhost().getArcs().at(arcId);
									dg.getGhost().unlink((GhostArc<T,GhostMode>*) arc);
								} catch (std::out_of_range) {
									// The arc was already deleted from an
									// other proc in the same epoch, so ignore
									// the operation.
								}
							}
						}
					}
			};
		template<typename T> void migrateUnlink(
				std::unordered_map<int, std::vector<DistributedId>> exportMap,
				DistributedGraphBase<T,GhostMode>& dg
				) {
			FPMAS::communication::migrate<
				DistributedId,
			DistributedGraphBase<T,GhostMode>,
			communication::voidPreMigrate<DistributedId, DistributedGraphBase<T,GhostMode>>,
			communication::voidMidMigrate<DistributedId, DistributedGraphBase<T,GhostMode>>,
			UnlinkPostMigrate<T>
				>(exportMap, dg.getMpiCommunicator(), dg);
		}
	}
}

#endif
