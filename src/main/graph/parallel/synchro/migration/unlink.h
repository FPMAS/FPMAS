#ifndef UNLINK_MIGRATION_H
#define UNLINK_MIGRATION_H

#include "communication/communication.h"
#include "communication/migrate.h"
#include "../../distributed_graph.h"

namespace FPMAS::graph::parallel {
	template<typename T, int N, SYNC_MODE> class GhostArc;
	namespace synchro {
		namespace modes {
			template<typename, int> class GhostMode;
		}
		using modes::GhostMode;
		template<typename T, int N> class UnlinkPostMigrate
			: public FPMAS::communication::PostMigrate<ArcId, DistributedGraph<T,GhostMode,N>> {
				public:
					void operator()(
							DistributedGraph<T,GhostMode,N>& dg, 
							std::unordered_map<int, std::vector<ArcId>> exportMap,
							std::unordered_map<int, std::vector<ArcId>> importMap
							) override {
						for(auto item : importMap) {
							for(ArcId arcId : item.second) {
								FPMAS_LOGD(
									dg.getMpiCommunicator().getRank(),
									"UNLINK_MIGRATE", "Imported unlink : %lu",
									arcId
									);
								try {
									auto arc = dg.getGhost().getArcs().at(arcId);
									dg.getGhost().unlink((GhostArc<T,N,GhostMode>*) arc);
								} catch (std::out_of_range) {
									// The arc was already deleted from an
									// other proc in the same epoch, so ignore
									// the operation.
								}
							}
						}
					}
			};
		template<typename T, int N> void migrateUnlink(
				std::unordered_map<int, std::vector<ArcId>> exportMap,
				DistributedGraph<T,GhostMode,N>& dg
				) {
			FPMAS::communication::migrate<
				ArcId,
			DistributedGraph<T,GhostMode,N>,
			communication::voidPreMigrate<ArcId, DistributedGraph<T,GhostMode,N>>,
			communication::voidMidMigrate<ArcId, DistributedGraph<T,GhostMode,N>>,
			UnlinkPostMigrate<T,N>
				>(exportMap, dg.getMpiCommunicator(), dg);
		}
	}
}

#endif
