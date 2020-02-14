#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "local_data.h"
#include "../proxy/proxy.h"
#include "../distributed_graph.h"

using FPMAS::graph::parallel::proxy::Proxy;


namespace FPMAS::graph::parallel {
	template<class T, SYNC_MODE, typename LayerType, int N> class DistributedGraph;

	using parallel::DistributedGraph;
}

	namespace FPMAS::graph::parallel::synchro {

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
		template<NODE_PARAMS> class GhostData : public SyncData<NODE_PARAMS_SPEC> {

			public:
				GhostData(unsigned long, SyncMpiCommunicator&, const Proxy&);
				GhostData(unsigned long, SyncMpiCommunicator&, const Proxy&, T);
				/**
				 * Defines the Zoltan configuration used manage and migrate
				 * GhostNode s and GhostArc s.
				 */
				const static zoltan::utils::zoltan_query_functions config;

				/**
				 * Termination function used at the end of each
				 * DistributedGraph<T,S>::synchronize() call. In this mode,
				 * ghost data is automatically updated from other procs.
				 */
				static void termination(DistributedGraph<T, GhostData, LayerType, N>* dg) {
					dg->getGhost().synchronize();
				}

		};
		template<NODE_PARAMS> const zoltan::utils::zoltan_query_functions GhostData<NODE_PARAMS_SPEC>::config
			(
			 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<NODE_PARAMS_SPEC, GhostData>,
			 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<NODE_PARAMS_SPEC, GhostData>,
			 &FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<NODE_PARAMS_SPEC, GhostData>
			);

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
		template<NODE_PARAMS> GhostData<NODE_PARAMS_SPEC>::GhostData(
				unsigned long id,
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
		template<NODE_PARAMS> GhostData<NODE_PARAMS_SPEC>::GhostData(
				unsigned long id,
				SyncMpiCommunicator& mpiComm,
				const Proxy& proxy,
				T data)
			: SyncData<NODE_PARAMS_SPEC>(data) {
			}
	}
#endif
