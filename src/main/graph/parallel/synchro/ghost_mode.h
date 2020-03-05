#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "../synchro/sync_mode.h"
#include "../proxy/proxy.h"
#include "../distributed_graph.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel {
	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	using parallel::DistributedGraph;

	namespace synchro {

		/**
		 * Synchronisation mode used as default by the DistributedGraph, and
		 * wrapper for distant data represented as "ghosts".
		 *
		 * In this mode, overlapping zones (represented as a "ghost graph")
		 * are built and synchronized on each proc.
		 *
		 * When a node is exported, ghost arcs and nodes are built to keep
		 * consistency across processes, and ghost nodes data is fetched
		 * at each DistributedGraph<T, S>::synchronize() call.
		 *
		 * Getters used are actually the defaults provided by SyncData and
		 * LocalData, because once data has been fetched from other procs, it
		 * can be accessed locally.
		 *
		 * In consequence, such "ghost" data can be read and written as any
		 * other LocalData. However, modifications **won't be reported** to the
		 * origin proc, and will be overriden at the next synchronization.
		 *
		 * @tparam wrapped data type
		 */
		template<class T> class GhostData : public SyncData<T> {
			
			public:
				GhostData(NodeId, SyncMpiCommunicator&, const Proxy&);
				GhostData(NodeId, SyncMpiCommunicator&, const Proxy&, const T&);
				GhostData(NodeId, SyncMpiCommunicator&, const Proxy&, T&&);

		};

		/**
		 * Builds a GhostData instance with the provided MpiCommunicator.
		 *
		 * @param id data id
		 * @param mpiComm MPI Communicator associated to the containing
		 * DistributedGraph
		 * @param proxy graph proxy
		 *
		 */
		// The mpiCommunicator is not used for the GhostData mode, but the
		// constructors are defined to allow template genericity
		template<class T> GhostData<T>::GhostData(
				NodeId id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy) {
		}

		/**
		 * Builds a GhostData instance initialized with the specified data.
		 *
		 * @param id data id
		 * @param data data to wrap
		 * @param mpiComm MPI Communicator associated to the containing
		 * DistributedGraph
		 * @param proxy graph proxy
		 */
		template<class T> GhostData<T>::GhostData(
				NodeId id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy,
				T&& data)
			: SyncData<T>(std::move(data)) {
			}

		template<class T> GhostData<T>::GhostData(
				NodeId id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy,
				const T& data)
			: SyncData<T>(data) {
			}

		struct NodeIdPairHash {
			std::size_t operator()(std::pair<NodeId, NodeId> const& pair) const {
				std::size_t h1 = std::hash<NodeId>()(pair.first);
				std::size_t h2 = std::hash<NodeId>()(pair.second);
				return h1 ^ h2;
			}
		};

		template<typename T, int N> class GhostMode : public SyncMode<GhostMode, GhostData, T, N> {
			private:
				std::unordered_map<
					std::pair<NodeId, NodeId>,
					Arc<std::unique_ptr<SyncData<T>>, N>*,
					NodeIdPairHash
				> linkBuffer;

			public:
			GhostMode();
			void termination(DistributedGraph<T, GhostMode, N>& dg);
			void initLink(NodeId, NodeId, ArcId, LayerId);
			void notifyLinked(Arc<std::unique_ptr<SyncData<T>>,N>*);
		};

		template<typename T, int N> GhostMode<T,N>::GhostMode() :
			SyncMode<GhostMode, GhostData, T,N>(zoltan_query_functions(
					&FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, N, GhostMode>,
					&FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, N, GhostMode>,
					&FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, N, GhostMode>
					))
		{}

		template<typename T, int N> void GhostMode<T, N>::termination(DistributedGraph<T, GhostMode, N>& dg) {
			dg.getGhost().synchronize();
		}

		template<typename T, int N> void GhostMode<T, N>::initLink(NodeId source, NodeId target, ArcId, LayerId) {
			// Clear unlink buffer

		}

		template<typename T, int N> void GhostMode<T, N>::notifyLinked(
				Arc<std::unique_ptr<SyncData<T>>,N>* arc) {
			linkBuffer[std::make_pair(
					arc->getSourceNode()->getId(), arc->getTargetNode()->getId()
					)]
				= arc;
		}

	}
}
#endif
