#ifndef NONE_H
#define NONE_H

#include "../zoltan/zoltan_utils.h"
#include "../zoltan/zoltan_node_migrate.h"
#include "graph/parallel/proxy/proxy.h"

using FPMAS::graph::parallel::proxy::Proxy;
/*
 *namespace FPMAS::graph::parallel::zoltan {
 *        namespace node {
 *            template<NODE_PARAMS, SYNC_MODE> void post_migrate_pp_fn_no_sync(
 *                    ZOLTAN_MID_POST_MIGRATE_ARGS
 *                    );
 *        }
 *        namespace arc {
 *            template<NODE_PARAMS, SYNC_MODE> void post_migrate_pp_fn_no_sync(
 *                    ZOLTAN_MID_POST_MIGRATE_ARGS
 *                    );
 *        }
 *    }
 */


namespace FPMAS::graph::parallel::synchro {
	/**
	 * Synchronisation mode that can be used as a template argument for
	 * a DistributedGraph, where no synchronization or overlapping zone
	 * is used.
	 *
	 * When a node is exported, its connections with nodes that are not
	 * exported on the same process are lost.
	 */
	template<NODE_PARAMS> class None : public SyncData<NODE_PARAMS_SPEC> {
		public:
			/**
			 * Zoltan config associated to the None synchronization
			 * mode.
			 */
			const static zoltan::utils::zoltan_query_functions config;

			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			None(unsigned long, SyncMpiCommunicator&, const Proxy&) {};
			/**
			 * Unused constructor, defined for synchronization mode API
			 * compatibility.
			 */
			None(unsigned long, SyncMpiCommunicator&, const Proxy&, T) {};

			/**
			 * Termination function used at the end of each
			 * DistributedGraph<T,S>::synchronize() call : does not do anything
			 * in this mode.
			 */
			static void termination(DistributedGraph<T, None, LayerType, N>* dg) {}
	};
	template<NODE_PARAMS> const zoltan::utils::zoltan_query_functions None<NODE_PARAMS_SPEC>::config
		(
		 &FPMAS::graph::parallel::zoltan::node::post_migrate_pp_fn_no_sync<NODE_PARAMS_SPEC>,
		 &FPMAS::graph::parallel::zoltan::arc::post_migrate_pp_fn_no_sync<NODE_PARAMS_SPEC>,
		 NULL
		);
}
#endif
