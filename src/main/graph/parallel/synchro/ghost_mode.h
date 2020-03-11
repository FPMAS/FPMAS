#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "utils/config.h"
#include "../synchro/sync_mode.h"
#include "../proxy/proxy.h"
#include "../distributed_graph.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel {
	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	using zoltan::utils::write_zoltan_id;
	using parallel::DistributedGraph;

	namespace synchro {
		template<
			template<typename, int> class,
			template<typename, int, SYNC_MODE> class,
			typename, int
		> class SyncMode;

		/**
		 * Data wrapper for the GhostMode.
		 *
		 * @tparam T wrapped data type
		 * @tparam N layers count
		 */
		template<typename T, int N, SYNC_MODE> class GhostData : public SyncData<T,N,S> {
			
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
		 * DistributedGraph
		 * @param proxy graph proxy
		 *
		 */
		// The mpiCommunicator is not used for the GhostData mode, but the
		// constructors are defined to allow template genericity
		template<typename T, int N, SYNC_MODE> GhostData<T,N,S>::GhostData(
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
		 * DistributedGraph
		 * @param proxy graph proxy
		 */
		template<typename T, int N, SYNC_MODE> GhostData<T,N,S>::GhostData(
				NodeId id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy,
				T&& data)
			: SyncData<T,N,S>(std::move(data)) {
			}

		/**
		 * Builds a GhostData instance initialized with a copy of the specified data.
		 *
		 * @param id data id
		 * @param data data to wrap
		 * @param mpiComm MPI Communicator associated to the containing
		 * DistributedGraph
		 * @param proxy graph proxy
		 */
		template<typename T, int N, SYNC_MODE> GhostData<T,N,S>::GhostData(
				NodeId id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy,
				const T& data)
			: SyncData<T,N,S>(data) {
			}

		/**
		 * Hash function object used to hase NodeId pairs.
		 */
		struct NodeIdPairHash {
			/**
			 * Hash operator.
			 */
			std::size_t operator()(std::pair<NodeId, NodeId> const& pair) const {
				std::size_t h1 = std::hash<NodeId>()(pair.first);
				std::size_t h2 = std::hash<NodeId>()(pair.second);
				return h1 ^ h2;
			}
		};

		/**
		 * Synchronisation mode used as default by the DistributedGraph.
		 *
		 * In this mode, overlapping zones (represented as a "ghost graph")
		 * are built and synchronized on each proc.
		 *
		 * When a node is exported, ghost arcs and nodes are built to keep
		 * consistency across processes, and ghost nodes data is fetched
		 * at each DistributedGraph::synchronize() call.
		 *
		 * Read and Acquire operations can directly access fetched data
		 * locally.
		 *
		 * In consequence, such "ghost" data can be read and written as any
		 * other local data. However, modifications **won't be reported** to the
		 * origin proc, and will be overriden at the next synchronization.
		 *
		 * Moreover, this mode allows to link or unlink nodes, according to the
		 * semantics defined in the DistributedGraph. Such graph modifications
		 * are imported and exported at each DistributedGraph::synchronize()
		 * call.
		 *
		 * @tparam wrapped data type
		 * @tparam N layers count
		 */
		template<typename T, int N> class GhostMode : public SyncMode<GhostMode, GhostData, T, N> {
			private:
				Zoltan zoltan;
				std::unordered_map<
					std::pair<NodeId, NodeId>,
					Arc<std::unique_ptr<SyncData<T,N,GhostMode>>, N>*,
					NodeIdPairHash
				> linkBuffer;
				int computeArcExportCount();

			public:
			GhostMode(DistributedGraph<T, GhostMode, N>&);
			void termination();
			void initLink(NodeId, NodeId, ArcId, LayerId);
			void notifyLinked(Arc<std::unique_ptr<SyncData<T,N,GhostMode>>,N>*);
		};

		/**
		 * GhostMode constructor.
		 *
		 * @param dg reference to the parent DistributedGraph
		 */
		template<typename T, int N> GhostMode<T,N>::GhostMode(DistributedGraph<T, GhostMode, N>& dg) :
			SyncMode<GhostMode, GhostData, T,N>(zoltan_query_functions(
					&FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, N, GhostMode>,
					&FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, N, GhostMode>,
					&FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, N, GhostMode>
					),
					dg)
		{
			FPMAS::config::zoltan_config(&this->zoltan);
		}

		/*
		 * Private computation to determine how many arcs need to be exported.
		 */
		template<typename T, int N> int GhostMode<T, N>::computeArcExportCount() {
			int n = 0;
			for(auto item : linkBuffer) {
				if(!this->dg.getProxy().isLocal(item.first.first) && !this->dg.getProxy().isLocal(item.first.second)) {
					n+=2;
				} else {
					n+=1;
				}
			}
			return n;
		}

		/**
		 * Called at each DistributedGraph::synchronize() call.
		 *
		 * Exports / imports link and unlink arcs, and fetch updated ghost data
		 * from other procs.
		 */
		template<typename T, int N> void GhostMode<T, N>::termination() {
			if(linkBuffer.size() > 0) {
				zoltan.Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T, N, GhostMode>, &this->dg);
				zoltan.Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T, N, GhostMode>, &this->dg);
				zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T, N, GhostMode>, &this->dg);

				int export_arcs_num = this->computeArcExportCount();
				ZOLTAN_ID_TYPE export_arcs_global_ids[2*export_arcs_num];
				int export_arcs_procs[export_arcs_num];
				ZOLTAN_ID_TYPE export_arcs_local_ids[0];

				int i=0;
				for(auto item : linkBuffer) {
					if(!this->dg.getProxy().isLocal(item.first.first)) {
						write_zoltan_id(item.second->getId(), &export_arcs_global_ids[2*i]);
						export_arcs_procs[i] = this->dg.getProxy().getCurrentLocation(item.first.first);
						i++;
					}
					if(!this->dg.getProxy().isLocal(item.first.second)) {
						write_zoltan_id(item.second->getId(), &export_arcs_global_ids[2*i]);
						export_arcs_procs[i] = this->dg.getProxy().getCurrentLocation(item.first.second);
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

			this->dg.getGhost().synchronize();
		}

		/**
		 * Initializes a distant link operation from `source` to `target` on
		 * the given `layer`.
		 *
		 * @param source source node id
		 * @param target target node id
		 * @param arcId new arc id
		 * @param layer layer id
		 */
		template<typename T, int N> void GhostMode<T, N>::initLink(NodeId source, NodeId target, ArcId arcId, LayerId layer) {
			// Clear unlink buffer

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
				Arc<std::unique_ptr<SyncData<T,N,GhostMode>>,N>* arc) {
			linkBuffer[std::make_pair(
					arc->getSourceNode()->getId(), arc->getTargetNode()->getId()
					)]
				= arc;
		}

	}
}
#endif
