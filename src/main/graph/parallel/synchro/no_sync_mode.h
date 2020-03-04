#ifndef NONE_H
#define NONE_H

#include "../zoltan/zoltan_utils.h"
#include "../zoltan/zoltan_node_migrate.h"
#include "graph/parallel/proxy/proxy.h"

using FPMAS::graph::parallel::proxy::Proxy;

namespace FPMAS::graph::parallel::synchro {

	/**
	 * Synchronisation mode that can be used as a template argument for
	 * a DistributedGraph, where no synchronization or overlapping zone
	 * is used.
	 *
	 * When a node is exported, its connections with nodes that are not
	 * exported on the same process are lost.
	 */
    template<class T> class NoSyncData : public SyncData<T> {
		public:
			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&) {};
			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&, T&& data) :
				SyncData<T>(std::move(data)) {};
			NoSyncData(NodeId, SyncMpiCommunicator&, const Proxy&, const T& data) :
				SyncData<T>(data) {};
	};

	template<typename T, int N> class NoSyncMode : public SyncMode<NoSyncMode, NoSyncData, T, N> {
		public:
		NoSyncMode();
	};

	template<typename T, int N> NoSyncMode<T,N>::NoSyncMode()
		: SyncMode<NoSyncMode, NoSyncData, T, N>(zoltan_query_functions(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_no_sync<T, N>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_no_sync<T, N>,
		 NULL
		 )) {};
}
#endif
