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
	template<class T> class None : public SyncData<T> {
		public:
			/**
			 * Zoltan config associated to the None synchronization
			 * mode.
			 */
			template<int N> const static zoltan::utils::zoltan_query_functions config;

			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			None(unsigned long, SyncMpiCommunicator&, const Proxy&) {};
			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			None(unsigned long, SyncMpiCommunicator&, const Proxy&, T&&) {};
			None(unsigned long, SyncMpiCommunicator&, const Proxy&, const T&) {};

			/**
			 * Termination function used at the end of each
			 * DistributedGraph<T,S>::synchronize() call : does not do anything
			 * in this mode.
			 */
			template<int N> static void termination(DistributedGraph<T, None, N>* dg) {}
	};
	template<class T> template<int N> const zoltan::utils::zoltan_query_functions None<T>::config
		(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_no_sync<T, N>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_no_sync<T, N>,
		 NULL
		);
}
#endif
