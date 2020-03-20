#ifndef UNLINK_MIGRATION_H
#define UNLINK_MIGRATION_H

#include "communication/communication.h"
#include "communication/migrate.h"
#include "../../distributed_graph_base.h"

namespace FPMAS::graph::parallel {
	template<typename T, int N, SYNC_MODE> class GhostArc;
	namespace synchro {
		namespace modes {
			template<typename, int> class GhostMode;
		}
		using modes::GhostMode;
		template<typename T, int N> class UnlinkPostMigrate
			: public FPMAS::communication::PostMigrate<IdType, DistributedGraphBase<T,GhostMode,N>> {
				public:
					void operator()(
							DistributedGraphBase<T,GhostMode,N>& dg, 
							std::unordered_map<int, std::vector<IdType>> exportMap,
							std::unordered_map<int, std::vector<IdType>> importMap
							) override {
						for(auto item : importMap) {
							for(IdType arcId : item.second) {
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
				std::unordered_map<int, std::vector<IdType>> exportMap,
				DistributedGraphBase<T,GhostMode,N>& dg
				) {
			FPMAS::communication::migrate<
				IdType,
			DistributedGraphBase<T,GhostMode,N>,
			communication::voidPreMigrate<IdType, DistributedGraphBase<T,GhostMode,N>>,
			communication::voidMidMigrate<IdType, DistributedGraphBase<T,GhostMode,N>>,
			UnlinkPostMigrate<T,N>
				>(exportMap, dg.getMpiCommunicator(), dg);
		}
	}
}

#endif
