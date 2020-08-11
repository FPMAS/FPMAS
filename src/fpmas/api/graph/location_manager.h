#ifndef LOCATION_MANAGER_API_H
#define LOCATION_MANAGER_API_H

#include "fpmas/api/graph/id.h"
#include "fpmas/api/graph/graph.h"
#include "distributed_id.h"
#include "fpmas/api/graph/distributed_node.h"

namespace fpmas {namespace api { namespace graph {

	/**
	 * LocationManager API.
	 *
	 * The purpose of the LocationManager is to provide an easy access to LOCAL
	 * and DISTANT Nodes within a local DistributedGraph, and to update DISTANT
	 * nodes locations so that DistributedNode::getLocation() always returns an
	 * updated value.
	 *
	 * Moreover, each LocationManager is assumed to maintain a "managed nodes"
	 * collection. When a node is "managed" by a LocationManager, other
	 * processes (and, more particularly, other LocationManager instances) can
	 * ask this LocationManager for the current location of the node.
	 * Notice that the fact that a node is managed by the LocationManager
	 * **does not mean that the node is located on the same process** than
	 * this LocationManager. It only means that this LocationManager _knows_
	 * where the node is currently located.
	 *
	 * Each DistributedNode in the DistributedGraph is assumed to be managed by
	 * exactly one LocationManager in the global execution.
	 */
	template<typename T>
	class LocationManager {
		public:
			typedef typename fpmas::api::graph::Graph<DistributedNode<T>, DistributedEdge<T>>::NodeMap NodeMap;

			/**
			 * Sets up the specified node to be managed by the LocationManager.
			 *
			 * @param node node to manage
			 * @param initial_location initial rank of the node
			 */
			virtual void addManagedNode(DistributedNode<T>* node, int initial_location) = 0;
			/**
			 * Removes a previously managed node.
			 *
			 * @param node node to remove from managed nodes
			 */
			virtual void removeManagedNode(DistributedNode<T>* node) = 0;

			/**
			 * Returns a NodeMap containing DistributedNodes that are LOCAL.
			 *
			 * More precisely, returns nodes whose last location update was
			 * performed with setLocal().
			 *
			 * @return LOCAL nodes map
			 */
			virtual const NodeMap& getLocalNodes() const = 0;

			/**
			 * Returns a NodeMap containing DistributedNodes that are DISTANT.
			 *
			 * More precisely, returns nodes whose last location update was
			 * performed with setDistant().
			 *
			 * @return DISTANT nodes map
			 */
			virtual const NodeMap& getDistantNodes() const = 0;

			/**
			 * Update the state of a node to local.
			 *
			 * Upon return, the state of the DistributedNode is set to LOCAL
			 * and its location is set to the current process rank.
			 *
			 * Moreover, the node will be returned by getLocalNodes().
			 *
			 * @param node node to set LOCAL
			 */
			virtual void setLocal(DistributedNode<T>* node) = 0;

			/**
			 * Update the state of a node to distant.
			 *
			 * Upon return, the state of the DistributedNode is set to DISTANT.
			 * The location of the node is undefined until it is updated by the
			 * next call to updateLocations().
			 *
			 * Moreover, the node will be returned by getDistantNodes().
			 *
			 * @param node node to set DISTANT
			 */
			virtual void setDistant(DistributedNode<T>* node) = 0;

			/**
			 * Removes a LOCAL or DISTANT node.
			 *
			 * Upon return, the node is neither returned by getLocalNodes() or
			 * getDistantNodes().
			 *
			 * @param node node to remove
			 */
			virtual void remove(DistributedNode<T>* node) = 0;

			/**
			 * Updates nodes locations.
			 *
			 * This functions is synchronous : it blocks until all
			 * LocationManager instances on each process call this function.
			 *
			 * Upon return, DistributedNode::getLocation() is assumed to return
			 * an updated value for **all the nodes** that where registered
			 * using setLocal() or setDistant().
			 */
			virtual void updateLocations() = 0;
	};
}}}
#endif
