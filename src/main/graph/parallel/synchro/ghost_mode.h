#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "utils/config.h"
#include "../synchro/sync_mode.h"
#include "../proxy/proxy.h"
#include "../olz.h"
#include "migration/unlink.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel {
	template<typename T, int N, SYNC_MODE> class GhostArc;
	template<typename T, SYNC_MODE, int N> class DistributedGraphBase;

	using zoltan::utils::write_zoltan_id;
	using parallel::DistributedGraphBase;

	namespace synchro {
		namespace modes {
			template<
				template<typename, int> class,
				template<typename, int> class,
				typename, int
					> class SyncMode;
			template<typename, int> class GhostMode;
		}

		namespace wrappers {

			/**
			 * Data wrapper for the GhostMode.
			 *
			 * @tparam T wrapped data type
			 * @tparam N layers count
			 */
			template<typename T, int N> class GhostData : public SyncData<T,N,modes::GhostMode> {

				public:
					GhostData(NodeId, SyncMpiCommunicator&, const Proxy&);
					GhostData(NodeId, SyncMpiCommunicator&, const Proxy&, const T&);
					GhostData(NodeId, SyncMpiCommunicator&, const Proxy&, T&&);

			};

			/**
			 * Builds a GhostData instance.
			 *
			 * @param id data id
			 * @param mpiComm MPI Communicator associated to the containing
			 * DistributedGraphBase
			 * @param proxy graph proxy
			 *
			 */
			// The mpiCommunicator is not used for the GhostData mode, but the
			// constructors are defined to allow template genericity
			template<typename T, int N> GhostData<T,N>::GhostData(
					NodeId id,
					SyncMpiCommunicator& mpiComm,
					const Proxy& proxy) {
			}

			/**
			 * Builds a GhostData instance initialized with the moved specified data.
			 *
			 * @param id data id
			 * @param data rvalue to data to wrap
			 * @param mpiComm MPI Communicator associated to the containing
			 * DistributedGraphBase
			 * @param proxy graph proxy
			 */
			template<typename T, int N> GhostData<T,N>::GhostData(
					NodeId id,
					SyncMpiCommunicator& mpiComm,
					const Proxy& proxy,
					T&& data)
				: SyncData<T,N,modes::GhostMode>(std::forward<T>(data)) {
				}

			/**
			 * Builds a GhostData instance initialized with a copy of the specified data.
			 *
			 * @param id data id
			 * @param data data to wrap
			 * @param mpiComm MPI Communicator associated to the containing
			 * DistributedGraphBase
			 * @param proxy graph proxy
			 */
			template<typename T, int N> GhostData<T,N>::GhostData(
					NodeId id,
					SyncMpiCommunicator& mpiComm,
					const Proxy& proxy,
					const T& data)
				: SyncData<T,N,modes::GhostMode>(data) {
				}
		}

		namespace modes {

			/**
			 * Synchronisation mode used as default by the DistributedGraphBase.
			 *
			 * In this mode, overlapping zones (represented as a "ghost graph")
			 * are built and synchronized on each proc.
			 *
			 * When a node is exported, ghost arcs and nodes are built to keep
			 * consistency across processes, and ghost nodes data is fetched
			 * at each DistributedGraphBase::synchronize() call.
			 *
			 * Read and Acquire operations can directly access fetched data
			 * locally.
			 *
			 * In consequence, such "ghost" data can be read and written as any
			 * other local data. However, modifications **won't be reported** to the
			 * origin proc, and will be overriden at the next synchronization.
			 *
			 * Moreover, this mode allows to link or unlink nodes, according to the
			 * semantics defined in the DistributedGraphBase. Such graph modifications
			 * are imported and exported at each DistributedGraphBase::synchronize()
			 * call.
			 *
			 * @tparam T wrapped data type
			 * @tparam N layers count
			 */
			template<typename T, int N> class GhostMode : public SyncMode<GhostMode, wrappers::GhostData, T, N> {
				private:
					Zoltan zoltan;
					std::unordered_map<
						ArcId,
						GhostArc<T, N, GhostMode>*
						> linkBuffer;
					std::unordered_map<int, std::vector<ArcId>> unlinkBuffer;
					int computeArcExportCount();
					void migrateLinks();
					void migrateUnlinks();

				public:
					GhostMode(DistributedGraphBase<T, GhostMode, N>&);
					void termination() override;

					void notifyLinked(
						Arc<std::unique_ptr<wrappers::SyncData<T,N,GhostMode>>,N>*
						) override;

					void notifyUnlinked(NodeId, NodeId, ArcId, LayerId) override;
			};

			/**
			 * GhostMode constructor.
			 *
			 * @param dg reference to the parent DistributedGraphBase
			 */
			template<typename T, int N> GhostMode<T,N>::GhostMode(DistributedGraphBase<T, GhostMode, N>& dg) :
				SyncMode<GhostMode, wrappers::GhostData, T,N>(zoltan_query_functions(
							&FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, N, GhostMode>,
							&FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, N, GhostMode>,
							&FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, N, GhostMode>
							),
						dg)
			{
				FPMAS::config::zoltan_config(&this->zoltan);
				zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, N, GhostMode>, &this->dg);
				zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, N, GhostMode>, &this->dg);
				zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, N, GhostMode>, &this->dg);
			}

			
			/**
			 * Called at each DistributedGraphBase::synchronize() call.
			 *
			 * Exports / imports link and unlink arcs, and fetch updated ghost data
			 * from other procs.
			 */
			template<typename T, int N> void GhostMode<T, N>::termination() {
				this->migrateLinks();
				this->migrateUnlinks();
				this->dg.getGhost().synchronize();
			}

			/**
			 * Called once a new distant `arc` has been created locally.
			 *
			 * The created `arc` is added to an internal buffer and migrated once
			 * termination() is called.
			 *
			 * @param arc pointer to the new arc to export
			 */
			template<typename T, int N> void GhostMode<T, N>::notifyLinked(
					Arc<std::unique_ptr<wrappers::SyncData<T,N,GhostMode>>,N>* arc) {
				linkBuffer[arc->getId()]
					= (GhostArc<T,N,GhostMode>*) arc;
			}

			template<typename T, int N> void GhostMode<T, N>::notifyUnlinked(
					NodeId source, NodeId target, ArcId arcId, LayerId layer
					) {
				FPMAS_LOGD(
					this->dg.getMpiCommunicator().getRank(),
					"GHOST_MODE", "Unlinking %lu : (%lu, %lu)",
					arcId, source, target
					);
				this->linkBuffer.erase(arcId);
				if(!this->dg.getProxy().isLocal(source)) {
					this->unlinkBuffer[
						this->dg.getProxy().getCurrentLocation(source)
					].push_back(arcId);
				}
				if(!this->dg.getProxy().isLocal(target)) {
					this->unlinkBuffer[
						this->dg.getProxy().getCurrentLocation(target)
					].push_back(arcId);
				}

			}

			template<typename T, int N> void GhostMode<T, N>::migrateLinks() {
				if(linkBuffer.size() > 0) {
					int export_arcs_num = this->computeArcExportCount();
					ZOLTAN_ID_TYPE export_arcs_global_ids[2*export_arcs_num];
					int export_arcs_procs[export_arcs_num];
					ZOLTAN_ID_TYPE export_arcs_local_ids[0];

					int i=0;
					for(auto item : linkBuffer) {
						NodeId sourceId = item.second->getSourceNode()->getId();
						if(!this->dg.getProxy().isLocal(sourceId)) {
							write_zoltan_id(item.first, &export_arcs_global_ids[2*i]);
							export_arcs_procs[i] = this->dg.getProxy().getCurrentLocation(sourceId);
							i++;
						}
						NodeId targetId = item.second->getTargetNode()->getId();
						if(!this->dg.getProxy().isLocal(targetId)) {
							write_zoltan_id(item.first, &export_arcs_global_ids[2*i]);
							export_arcs_procs[i] = this->dg.getProxy().getCurrentLocation(targetId);
							i++;
						}
					}

					this->zoltan.Migrate(
							-1,
							NULL,
							NULL,
							NULL,
							NULL,
							export_arcs_num,
							export_arcs_global_ids,
							export_arcs_local_ids,
							export_arcs_procs,
							export_arcs_procs // parts = procs
							);
				}

				for(auto item : linkBuffer) {
					NodeId sourceId = item.second->getSourceNode()->getId();
					NodeId targetId = item.second->getTargetNode()->getId();
					if(!this->dg.getProxy().isLocal(sourceId)
							&& !this->dg.getProxy().isLocal(targetId)) {
						this->dg.getGhost().unlink((GhostArc<T,N,GhostMode>*) item.second);
					}
				}
				linkBuffer.clear();
			}

			/*
			 * Private computation to determine how many arcs need to be exported.
			 */
			template<typename T, int N> int GhostMode<T, N>::computeArcExportCount() {
				int n = 0;
				for(auto item : linkBuffer) {
					if(!this->dg.getProxy().isLocal(
								item.second->getSourceNode()->getId()
							) && !this->dg.getProxy().isLocal(
								item.second->getTargetNode()->getId()
							)) {
						// The link must be exported to the two procs
						n+=2;
					} else {
						// Sent only to target or source location proc
						n+=1;
					}
				}
				return n;
			}

			template<typename T, int N> void GhostMode<T, N>::migrateUnlinks() {
				synchro::migrateUnlink<T, N>(
					this->unlinkBuffer, this->dg
					);
				this->unlinkBuffer.clear();
			}

		}
	}
}
#endif
