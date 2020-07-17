#ifndef EDGE_API_H
#define EDGE_API_H

#include "id.h"

namespace fpmas { namespace api { namespace graph {

	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	/**
	 * Graph edge
	 *
	 * An Edge represents a link between two nodes on a given layer.
	 */
	template<typename _IdType, typename _NodeType>
	class Edge {
		public:
			/**
			 * ID type
			 */
			typedef _IdType IdType;
			/**
			 * Node type
			 */
			typedef _NodeType NodeType;

			/**
			 * ID of the Edge.
			 *
			 * @return edge's ID
			 */
			virtual IdType getId() const = 0;

			/**
			 * Layer of the Edge.
			 *
			 * Layers are used to subset incoming and outgoing edges lists in
			 * the following Node's functions:
			 * - Node::getIncomingEdges(LayerId) const
			 * - Node::inNeighbors(LayerId) const
			 * - Node::getOutgoingEdges(LayerId) const
			 * - Node::inNeighbors(LayerId) const
			 *
			 * @return edge's layer
			 */
			virtual LayerId getLayer() const = 0;

			/**
			 * Weight of the Edge.
			 *
			 * The edge's weight might be used to perform LoadBalancing.
			 *
			 * @return edge's weight
			 */
			virtual float getWeight() const = 0;

			/**
			 * Sets the weight of the Edge.
			 *
			 * @param weight new weight
			 */
			virtual void setWeight(float weight) = 0;

			/**
			 * Sets the source node of this edge.
			 *
			 * @param src pointer to source node
			 */
			virtual void setSourceNode(NodeType* const src) = 0;

			/**
			 * Source node of this edge.
			 *
			 * @return pointer to source node
			 */
			virtual NodeType* const getSourceNode() const = 0;

			/**
			 * Sets the target node of this edge.
			 *
			 * @param tgt pointer to target node
			 */
			virtual void setTargetNode(NodeType* const tgt) = 0;

			/**
			 * Target node of this edge.
			 *
			 * @return pointer to target node
			 */
			virtual NodeType* const getTargetNode() const = 0;

			virtual ~Edge() {};
	};
}}}
#endif
