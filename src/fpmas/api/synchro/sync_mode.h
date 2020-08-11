#ifndef SYNC_MODE_API_H
#define SYNC_MODE_API_H

#include <string>
#include "fpmas/api/graph/distributed_graph.h"

namespace fpmas { namespace api { namespace synchro {
	/**
	 * Data synchronization API.
	 *
	 * The behavior of this component can be very different depending on the
	 * implemented synchronization mode. See the corresponding implementations
	 * for more details.
	 */
	class DataSync {
		public:
			/**
			 * Synchronizes api::graph::DistributedGraph data across the
			 * processes, i.e. the internal data of each
			 * api::graph::DistributedNode, or the managed data of each Mutex.
			 */
			virtual void synchronize() = 0;
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
	 */
	template<typename T>
	class SyncLinker {
		public:
			/**
			 * Notifies the specified edge has been linked in the local graph.
			 */
			virtual void link(const api::graph::DistributedEdge<T>* edge) = 0;
			/**
			 * Notifies the specified edge has been unlinked in the local graph.
			 */
			virtual void unlink(const api::graph::DistributedEdge<T>* edge) = 0;

			/**
			 * Synchronizes link, unlink and node removal operations across
			 * the processes.
			 */
			virtual void synchronize() = 0;
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
				virtual DataSync& getDataSync() = 0;
		};
}}}
#endif
