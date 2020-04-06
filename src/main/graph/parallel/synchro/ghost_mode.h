#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "utils/config.h"
#include "../synchro/sync_mode.h"
#include "../proxy/proxy.h"
#include "../olz.h"
#include "migration/unlink.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel {
	template<typename T, SYNC_MODE> class GhostArc;
	template<typename T, SYNC_MODE> class DistributedGraphBase;

	using zoltan::utils::write_zoltan_id;
	using parallel::DistributedGraphBase;

	namespace synchro {
		namespace modes {
			template<
				template<typename> class,
				template<typename> class,
				typename
					> class SyncMode;
			template<typename> class GhostMode;
		}

		namespace wrappers {

			/**
			 * Data wrapper for the GhostMode.
			 *
			 * @tparam T wrapped data type
			 * @tparam N layers count
			 */
			template<typename T> class GhostData : public SyncData<T,modes::GhostMode> {
				typedef api::communication::RequestHandler request_handler;

				public:
					GhostData(DistributedId, request_handler&, const Proxy&);
					GhostData(DistributedId, request_handler&, const Proxy&, const T&);
					GhostData(DistributedId, request_handler&, const Proxy&, T&&);

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
			template<typename T> GhostData<T>::GhostData(
					DistributedId id,
					request_handler& requestHandler,
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
			template<typename T> GhostData<T>::GhostData(
					DistributedId id,
					request_handler& requestHandler,
					const Proxy& proxy,
					T&& data)
				: SyncData<T,modes::GhostMode>(std::forward<T>(data)) {
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
			template<typename T> GhostData<T>::GhostData(
					DistributedId id,
					request_handler& requestHandler,
					const Proxy& proxy,
					const T& data)
				: SyncData<T,modes::GhostMode>(data) {
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
			template<typename T> class GhostMode : public SyncMode<GhostMode, wrappers::GhostData, T> {
				private:
					Zoltan zoltan;
					std::unordered_map<
						DistributedId,
						GhostArc<T, GhostMode>*
						> linkBuffer;
					std::unordered_map<int, std::vector<DistributedId>> unlinkBuffer;
					int computeArcExportCount();
					void migrateLinks();
					void migrateUnlinks();

				public:
					GhostMode(DistributedGraphBase<T, GhostMode>&);
					void termination() override;

					void notifyLinked(
						Arc<std::unique_ptr<wrappers::SyncData<T,GhostMode>>,DistributedId>*
						) override;

					void notifyUnlinked(DistributedId, DistributedId, DistributedId, LayerId) override;
			};

			/**
			 * GhostMode constructor.
			 *
			 * @param dg reference to the parent DistributedGraphBase
			 */
			template<typename T> GhostMode<T>::GhostMode(DistributedGraphBase<T, GhostMode>& dg) :
				SyncMode<GhostMode, wrappers::GhostData, T>(zoltan_query_functions(
							&FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, GhostMode>,
							&FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, GhostMode>,
							&FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, GhostMode>
							),
						dg)
			{
				FPMAS::config::zoltan_config(&this->zoltan);
				zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, GhostMode>, &this->dg);
				zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, GhostMode>, &this->dg);
				zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, GhostMode>, &this->dg);
			}

			
			/**
			 * Called at each DistributedGraphBase::synchronize() call.
			 *
			 * Exports / imports link and unlink arcs, and fetch updated ghost data
			 * from other procs.
			 */
			template<typename T> void GhostMode<T>::termination() {
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
			template<typename T> void GhostMode<T>::notifyLinked(
					Arc<std::unique_ptr<wrappers::SyncData<T,GhostMode>>,DistributedId>* arc) {
				FPMAS_LOGD(
					this->dg.getMpiCommunicator().getRank(),
					"GHOST_MODE", "Linking %s : (%s, %s)",
					ID_C_STR(arc->getId()), ID_C_STR(arc->getSourceNode()->getId()), ID_C_STR(arc->getTargetNode()->getId())
					);
				linkBuffer[arc->getId()]
					= (GhostArc<T,GhostMode>*) arc;
			}

			/**
			 * Called once a distant `arc`, linking `source` to `target`, is
			 * unlinked.
			 *
			 * The unlinked arc is added to an internal buffer, and migrated
			 * once termination() is called.
			 *
			 * Additionally, if the unlinked arc is an arc that were created
			 * but not migrated yet, it is removed from the corresponding
			 * internal buffer to avoid unnecessary distant operations.
			 *
			 * @param source source id
			 * @param target target id
			 * @param arcId id of the arc to unlink
			 * @param layer layer of the unlinked arc
			 */
			template<typename T> void GhostMode<T>::notifyUnlinked(
					DistributedId source, DistributedId target, DistributedId arcId, LayerId layer
					) {
				FPMAS_LOGD(
					this->dg.getMpiCommunicator().getRank(),
					"GHOST_MODE", "Unlinking %s : (%s, %s)",
					ID_C_STR(arcId), ID_C_STR(source), ID_C_STR(target)
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

			template<typename T> void GhostMode<T>::migrateLinks() {
				int export_arcs_num = this->computeArcExportCount();
				ZOLTAN_ID_TYPE export_arcs_global_ids[2*export_arcs_num];
				int export_arcs_procs[export_arcs_num];
				ZOLTAN_ID_TYPE export_arcs_local_ids[0];

				int i=0;
				for(auto item : linkBuffer) {
					DistributedId sourceId = item.second->getSourceNode()->getId();
					if(!this->dg.getProxy().isLocal(sourceId)) {
						write_zoltan_id(item.first, &export_arcs_global_ids[2*i]);
						export_arcs_procs[i] = this->dg.getProxy().getCurrentLocation(sourceId);
						i++;
					}
					DistributedId targetId = item.second->getTargetNode()->getId();
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

				for(auto item : linkBuffer) {
					DistributedId sourceId = item.second->getSourceNode()->getId();
					DistributedId targetId = item.second->getTargetNode()->getId();
					if(!this->dg.getProxy().isLocal(sourceId)
							&& !this->dg.getProxy().isLocal(targetId)) {
						this->dg.getGhost().unlink((GhostArc<T,GhostMode>*) item.second);
					}
				}
				linkBuffer.clear();
			}

			/*
			 * Private computation to determine how many arcs need to be exported.
			 */
			template<typename T> int GhostMode<T>::computeArcExportCount() {
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

			template<typename T> void GhostMode<T>::migrateUnlinks() {
				synchro::migrateUnlink<T>(
					this->unlinkBuffer, this->dg
					);
				this->unlinkBuffer.clear();
			}

		}
	}
}
#endif
