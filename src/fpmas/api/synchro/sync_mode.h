#ifndef FPMAS_SYNC_MODE_API_H
#define FPMAS_SYNC_MODE_API_H

/** \file src/fpmas/api/synchro/sync_mode.h
 * SyncMode API
 */

#include "mutex.h"

#include <unordered_set>

namespace fpmas { namespace api { namespace graph {
	template<typename T> class DistributedNode;
	template<typename T> class DistributedEdge;
}}}

namespace fpmas { namespace api { namespace synchro {
	/**
	 * Data synchronization API.
	 *
	 * The behavior of this component can be very different depending on the
	 * implemented synchronization mode. See the corresponding implementations
	 * for more details.
	 */
	template<typename T>
		class DataSync {
			public:
				/**
				 * Synchronizes api::graph::DistributedGraph data across the
				 * processes, i.e. the internal data of each
				 * api::graph::DistributedNode, or the managed data of each Mutex.
				 */
				virtual void synchronize() = 0;

				/**
				 * Performs a partial synchronization of the specified node
				 * set.
				 *
				 * The actual behavior is highly dependent on the currently
				 * implemented SyncMode.
				 *
				 * The specified `nodes` list might contain \LOCAL and \DISTANT
				 * nodes: it is the responsibility of the implemented SyncMode
				 * to eventually not do anything with \LOCAL nodes.
				 *
				 * @param nodes nodes to synchronize
				 */
				virtual void synchronize(
						std::unordered_set<api::graph::DistributedNode<T>*>
						nodes) = 0;

				virtual ~DataSync() {}
		};

	/**
	 * Synchronized linker API.
	 *
	 * The behavior of this component can be very different depending on the
	 * implemented synchronization mode. See the corresponding implementations
	 * for more details.
	 *
	 * Notified link, unlink or node removal operations correspond to
	 * intentional user operations. Internal and low-level operations that
	 * might occur while distributing the graph for example are not notified to
	 * the SyncLinker.
	 *
	 * Whatever the synchronization mode is, it is guaranteed that when the
	 * next synchronize() call returns, all notified operations are committed
	 * and applied globally.
	 *
	 * For example, if removeNode() is called with a \DISTANT node as argument,
	 * the local representation of the node (and its linked \DISTANT edges)
	 * might be deleted from the local graph, but it is guaranteed that the
	 * Node and its linked edges are globally removed from the global graph
	 * (i.e.  also from other processes, including the process that owns the
	 * node) only when synchronize() is called.
	 */
	template<typename T>
	class SyncLinker {
		public:
			/**
			 * Notifies the specified edge has been linked.
			 *
			 * @param edge linked edge
			 */
			virtual void link(api::graph::DistributedEdge<T>* edge) = 0;
			/**
			 * Notifies the specified edge must be unlinked.
			 *
			 * @param edge edge to unlink
			 */
			virtual void unlink(api::graph::DistributedEdge<T>* edge) = 0;
			/**
			 * Notifies the specified node must be removed.
			 *
			 * @param node node to remove
			 */
			virtual void removeNode(api::graph::DistributedNode<T>* node) = 0;

			/**
			 * Synchronizes link, unlink and node removal operations across
			 * the processes.
			 */
			virtual void synchronize() = 0;

			virtual ~SyncLinker() {}
	};

	/**
	 * SyncMode API.
	 *
	 * A synchronization mode is mainly defined by 3 components :
	 * - A Mutex type, that protects internal nodes data and allows access to
	 *   it.
	 * - A DataSync type, that ensures node data synchronization.
	 * - A SyncLinker type, that allows synchronized link, unlink and node removal
	 *   operations.
	 */
	template<typename T>
		class SyncMode {
			public:
				/**
				 * Builds a Mutex associated to the specified node.
				 *
				 * The Mutex must be built so that the Mutex::data() function
				 * returns a reference to the internal node data, i.e.
				 * api::graph::DistributedNode::data().
				 *
				 * Notice that this function is not required to bind the
				 * built Mutex to the node, since this might be the role of the
				 * component in charge to build the node (i.e. the
				 * api::graph::DistributedGraph implementation).
				 *
				 * @param node node to which the built mutex will be associated
				 */
				virtual Mutex<T>* buildMutex(api::graph::DistributedNode<T>* node) = 0;

				/**
				 * Returns a reference to the SyncLinker instance associated to
				 * this synchronization mode.
				 *
				 * @return sync linker
				 */
				virtual SyncLinker<T>& getSyncLinker() = 0;

				/**
				 * Returns a reference to the DataSync instance associated to
				 * this synchronization mode.
				 *
				 * @return data sync
				 */
				virtual DataSync<T>& getDataSync() = 0;

				virtual ~SyncMode() {};
		};
}}}
#endif
