#ifndef FPMAS_NODE_API_H
#define FPMAS_NODE_API_H

/** \file src/fpmas/api/graph/node.h
 * Node API
 */

#include <vector>
#include "fpmas/api/graph/edge.h"

namespace fpmas { namespace api { namespace graph {

	/**
	 * Graph node
	 */
	template<typename _IdType, typename _EdgeType>
		class Node {
			public:
				/**
				 * ID type
				 */
				typedef _IdType IdType;
				/**
				 * Edge type
				 */
				typedef _EdgeType EdgeType;

				/**
				 * ID of the Node.
				 *
				 * @return node's ID
				 */
				virtual IdType getId() const = 0;

				/**
				 * Weight of the Node.
				 *
				 * The node's weight might be used to perform LoadBalancing.
				 *
				 * @return node's weight
				 */
				virtual float getWeight() const = 0;

				/**
				 * Sets the weight of the Node.
				 *
				 * @param weight new weight
				 */
				virtual void setWeight(float weight) = 0;

				/**
				 * Returns a vector containing pointers to **all** the incoming
				 * edges of this node.
				 *
				 * @return pointers to incoming edges
				 */
				virtual const std::vector<EdgeType*> getIncomingEdges() const = 0;

				/**
				 * Returns a vector containing pointers to the incoming
				 * edges of this node on the specified layer.
				 *
				 * @param layer_id layer ID
				 * @return pointers to incoming edges
				 */
				virtual const std::vector<EdgeType*> getIncomingEdges(LayerId layer_id) const = 0;

				/**
				 * Returns a vector containing pointers to **all** the nodes
				 * connected to this node with an **incoming** edge, as
				 * returned by getIncomingEdges().
				 *
				 * @return pointers to incoming neighbors
				 */
				virtual const std::vector<typename EdgeType::NodeType*> inNeighbors() const = 0;

				/**
				 * Returns a vector containing pointers to the nodes
				 * connected to this node with an **incoming** edge on the
				 * specified layer, as returned by getIncomingEdges(layer_id).
				 *
				 * @param layer_id layer ID
				 * @return pointers to incoming neighbors
				 */
				virtual const std::vector<typename EdgeType::NodeType*> inNeighbors(LayerId layer_id) const = 0;

				/**
				 * Returns a vector containing pointers to **all** the outgoing
				 * edges of this node.
				 *
				 * @return pointers to outgoing edges
				 */
				virtual const std::vector<EdgeType*> getOutgoingEdges() const = 0;

				/**
				 * Returns a vector containing pointers to the outgoing
				 * edges of this node on the specified layer.
				 *
				 * @param layer_id layer ID
				 * @return pointers to outgoing edges
				 */
				virtual const std::vector<EdgeType*> getOutgoingEdges(LayerId layer_id) const = 0;

				/**
				 * Returns a vector containing pointers to **all** the nodes
				 * connected to this node with an **outgoing** edge, as
				 * returned by getOutgoingEdges().
				 *
				 * @return pointers to outgoing neighbors
				 */
				virtual const std::vector<typename EdgeType::NodeType*> outNeighbors() const = 0;

				/**
				 * Returns a vector containing pointers to the nodes
				 * connected to this node with an **outgoing** edge on the
				 * specified layer, as returned by getOutgoingEdges(layer_id).
				 *
				 * @param id layer ID
				 * @return pointers to outgoing neighbors
				 */
				virtual const std::vector<typename EdgeType::NodeType*> outNeighbors(LayerId layer_id) const = 0;

				/**
				 * Links the specified incoming edge to this node.
				 *
				 * Once the node is linked, it should be returned in the
				 * following lists :
				 * - getIncomingEdges()
				 * - getIncomingEdges(edge->getLayer())
				 *
				 * @param edge incoming edge to link
				 */
				virtual void linkIn(EdgeType* edge) = 0;

				/**
				 * Links the specified outgoing edge to this node.
				 *
				 * Once the node is linked, it should be returned in the
				 * following lists :
				 * - getOutgoingEdges()
				 * - getOutgoingEdges(edge->getLayer())
				 *
				 * @param edge outgoing edge to link
				 */
				virtual void linkOut(EdgeType* edge) = 0;

				/**
				 * Unlinks the specified incoming edge from this node.
				 *
				 * Once the node is unlinked, it should **not** be returned anymore by the
				 * following functions :
				 * - getIncomingEdges()
				 * - getIncomingEdges(edge->getLayer())
				 *
				 * @param edge incoming edge to unlink
				 */
				virtual void unlinkIn(EdgeType* edge) = 0;

				/**
				 * Unlinks the specified outgoing edge from this node.
				 *
				 * Once the node is unlinked, it should **not** be returned anymore by the
				 * following functions :
				 * - getOutgoingEdges()
				 * - getOutgoingEdges(edge->getLayer())
				 *
				 * @param edge outgoing edge to unlink
				 */
				virtual void unlinkOut(EdgeType* edge) = 0;

				virtual ~Node() {}
		};
}}}
#endif
