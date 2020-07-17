#ifndef DISTRIBUTED_NODE_API_H
#define DISTRIBUTED_NODE_API_H

#include "fpmas/api/graph/node.h"
#include "fpmas/api/graph/distributed_edge.h"
#include "fpmas/api/synchro/mutex.h"
#include "distributed_id.h"

namespace fpmas { namespace api { namespace graph {

	/**
	 * DistributedNode API.
	 *
	 * The DistributedNode is an extension of the Node API, specialized using
	 * DistributedId and DistributedEdge, and introduces some distribution
	 * related concepts.
	 *
	 * The class also introduces T, the type of the data contained in the
	 * DistributedNode. Each DistributedNode instance owns an instance of T.
	 *
	 * @tparam T data type
	 */
	template<typename T>
	class DistributedNode
		: public virtual fpmas::api::graph::Node<DistributedId, DistributedEdge<T>> {
		typedef fpmas::api::graph::Node<DistributedId, DistributedEdge<T>> NodeBase;

		public:

			/**
			 * Rank of the process on which this node is currently located.
			 *
			 * If the node is LOCAL, this rank corresponds to the current
			 * process.
			 *
			 * @return node location
			 */
			virtual int getLocation() const = 0;

			/**
			 * Updates the location of the node.
			 *
			 * Only used for internal / serialization.
			 *
			 * @param location new location
			 */
			virtual void setLocation(int location) = 0;

			/**
			 * Current state of the node.
			 *
			 * A DistributedNode is LOCAL iff it is currently hosted and managed by the current
			 * process.
			 * A DISTANT DistributedNode correspond to a representation of a
			 * node currently hosted by an other process.
			 *
			 * @return current node state
			 */
			virtual LocationState state() const = 0;

			/**
			 * Updates the state of the node.
			 *
			 * Only intended for internal / serialization usage.
			 *
			 * @param state new state
			 */
			virtual void setState(LocationState state) = 0;

			/**
			 * Returns a reference to the internal node data.
			 *
			 * @return reference to node's data
			 */
			virtual T& data() = 0;
			/**
			 * \copydoc data()
			 */
			virtual const T& data() const = 0;


			/**
			 * Sets the internal mutex of the node.
			 *
			 * The concrete mutex type used might depend on the graph
			 * synchronization mode.
			 *
			 * @param mutex node mutex
			 */
			virtual void setMutex(synchro::Mutex<T>* mutex) = 0;

			/**
			 * Internal node mutex.
			 *
			 * The role of the mutex is to protect the access to the internal T
			 * instance. The data references return by the Mutex methods
			 * correspond the node's internal T instance.
			 *
			 * @return pointer to the internal node mutex
			 */
			virtual synchro::Mutex<T>* mutex() = 0;

			/**
			 * \copydoc mutex()
			 */
			virtual const synchro::Mutex<T>* mutex() const = 0;

			virtual ~DistributedNode() {}
	};

}}}
#endif
