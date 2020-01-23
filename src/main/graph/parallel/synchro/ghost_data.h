#ifndef GHOST_DATA_H
#define GHOST_DATA_H

#include "local_data.h"

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
	 * at each GhostGraph::synchronize() call.
	 *
	 * Getters used are actually the defaults provided by SyncData and
	 * LocalData, because once data has been fetched from other procs, it
	 * can be accessed locally.
	 *
	 * In consequence, such "ghost" data can be read and written as any
	 * other LocalData. However, modifications **won't be reported** to the
	 * origin proc, and will be overriden at the next synchronization.
	 *
	 */
	template<class T> class GhostData : public LocalData<T> {
		private:
			TerminableMpiCommunicator& mpiComm;

		public:
			GhostData(TerminableMpiCommunicator&);
			GhostData(TerminableMpiCommunicator&, T);

			/**
			 * Defines the Zoltan configuration used manage and migrate
			 * GhostNode s and GhostArc s.
			 */
			const static zoltan::utils::zoltan_query_functions config;

	};
	template<class T> const zoltan::utils::zoltan_query_functions GhostData<T>::config
		(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_olz<T, GhostData>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_olz<T, GhostData>,
		 &FPMAS::graph::parallel::zoltan::arc::mid_migrate_pp_fn<T, GhostData>
		);

	/**
	 * Builds a GhostData instance with the provided MpiCommunicator.
	 *
	 * @param mpiComm MPI Communicator associated to the containing
	 * DistributedGraph
	 */
	// The mpiCommunicator is not used for the GhostData mode, but the
	// constructors are defined to allow template genericity
	template<class T> GhostData<T>::GhostData(TerminableMpiCommunicator& mpiComm)
		: mpiComm(mpiComm) {}

	/**
	 * Builds a GhostData instance initialized with the specified data.
	 *
	 * @param data data to wrap
	 * @param mpiComm MPI Communicator associated to the containing
	 * DistributedGraph
	 */
	template<class T> GhostData<T>::GhostData(TerminableMpiCommunicator& mpiComm, T data)
		: LocalData<T>(data), mpiComm(mpiComm) {}
}
#endif
