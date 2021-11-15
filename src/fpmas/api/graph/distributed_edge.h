#ifndef FPMAS_DISTRIBUTED_EDGE_API_H
#define FPMAS_DISTRIBUTED_EDGE_API_H

/** \file src/fpmas/api/graph/distributed_edge.h
 * DistributedEdge API
 */

#include "fpmas/api/graph/edge.h"
#include "distributed_id.h"
#include "location_state.h"

namespace fpmas {namespace api { namespace graph {
#ifndef DOXYGEN_BUILD
	template<typename> class DistributedNode;
#endif

	/**
	 * TemporaryNode API.
	 *
	 * A temporary node can be used to optimize DistributedEdge
	 * deserialization. Indeed, when a node is imported in a DistributedGraph
	 * for example, at least the source or target DistributedNode should
	 * already be contained in the graph. In consequence, at least source or
	 * target node of the imported edge will not be used before being discarded
	 * and replaced by a local DistributedNode: deserializing and allocating
	 * such nodes can represent a huge waste of time and memory resources (what
	 * has been observed experimentally).
	 *
	 * This can be solved storing temporary source and target nodes has
	 * TemporaryNodes in a deserialized DistributedEdge. If a temporary node is
	 * finally useless, there is no need to deserialize it or to allocate it,
	 * and the TemporaryNode is simply discarded without any useless memory
	 * usage. If the temporary node is required (for example, because it is not
	 * already contained in the graph), is can be deserialized and allocated
	 * **only when required** using the build() method.
	 *
	 * @see DistributedEdge::setTempSourceNode()
	 * @see DistributedEdge::getTempSourceNode()
	 * @see DistributedEdge::setTempTargetNode()
	 * @see DistributedEdge::getTempTargetNode()
	 * @see DistributedGraph::importEdge()
	 * @see @ref adl_serializer_DistributedEdge "nlohmann::adl_serializer<DistributedEdge>"
	 *
	 * @tparam T node data type
	 */
	template<typename T>
		class TemporaryNode {
			public:
				/**
				 * Gets the id of the node represented by the TemporaryNode.
				 *
				 * This should not require the complete node deserialization
				 * and allocation.
				 *
				 * @return temporary node id
				 */
				virtual DistributedId getId() const = 0;

				/**
				 * Gets the processor rank where the node represented by the
				 * TemporaryNode is located.
				 *
				 * @return node location
				 */
				virtual int getLocation() const = 0;

				/**
				 * Desesializes (if required) and allocates the DistributedNode
				 * represented by this temporary node and returns it.
				 *
				 * The node ID must be the value of getId(), and its location
				 * must be initialized to getLocation().
				 */
				virtual DistributedNode<T>* build() = 0;

				virtual ~TemporaryNode() {
				}
		};

	/**
	 * DistributedEdge API.
	 *
	 * The DistributedEdge is an extension of the Edge API, specialized using
	 * DistributedId and DistributedNode, and introduces some distribution
	 * related concepts.
	 *
	 * @tparam T associated node data type
	 */
	template<typename T>
		class DistributedEdge
		: public virtual fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> {
			typedef fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> EdgeBase;
			public:
			/**
			 * Current state of the edge.
			 *
			 * A DistributedEdge is \LOCAL iff its source and target nodes are
			 * \LOCAL.
			 *
			 * @return current edge state
			 */
			virtual LocationState state() const = 0;
			/**
			 * Updates the state of the edge.
			 *
			 * Only intended for internal / serialization usage.
			 *
			 * @param state new state
			 */
			virtual void setState(LocationState state) = 0;

			/**
			 * Sets the temporary source node of this DistributedEdge.
			 *
			 * The `temp_src` unique_ptr is _moved_ internally so that the
			 * current DistributedEdge instance takes ownership of it.
			 *
			 * This unique_ptr instance can be retrieve from
			 * getTempSourceNode().
			 *
			 * @param temp_src pointer to temporary source node
			 */
			virtual void setTempSourceNode(std::unique_ptr<api::graph::TemporaryNode<T>> temp_src) = 0;

			/**
			 * Gets the current temporary source node attached to this
			 * DistributedEdge.
			 *
			 * The caller takes ownership of the unique_ptr, leaving the
			 * current temporary source node in a null state.
			 *
			 * @warning
			 * Ownership of temporary nodes should be manages carefully.
			 * Indeed, the following example would produce a segmentation
			 * fault, since the unique_ptr would be deleted at the first
			 * `getTempSourceNode()` call:
			 * ```cpp
			 * auto id = edge->getTempSourceNode()->getId();
			 * auto edge = edge->getTempSourceNode()->build();
			 * ```
			 *
			 * However, the following is valid:
			 * ```cpp
			 * // Takes ownership of the temporary node
			 * {
			 *     auto temp_node = edge->getTempSourceNode();
			 *     auto id = temp_node->getId();
			 *     auto node = temp_node->build();
			 *     // temp_node automatically deleted at the end of the scope
			 * }
			 * ```
			 *
			 * @return current temporary source node. Might be null.
			 */
			virtual std::unique_ptr<TemporaryNode<T>> getTempSourceNode() = 0;

			/**
			 * Sets the temporary target node of this DistributedEdge.
			 *
			 * The `temp_tgt` unique_ptr is _moved_ internally so that the
			 * current DistributedEdge instance takes ownership of it.
			 *
			 * This unique_ptr instance can be retrieve from
			 * getTempTargetNode().
			 *
			 * @param temp_tgt pointer to temporary target node
			 */
			virtual void setTempTargetNode(std::unique_ptr<api::graph::TemporaryNode<T>> temp_tgt) = 0;

			/**
			 * Gets the current temporary target node attached to this
			 * DistributedEdge.
			 *
			 * The caller takes ownership of the unique_ptr, leaving the
			 * current temporary target node in a null state.
			 *
			 * @warning
			 * See getTempSourceNode().
			 *
			 * @return current temporary target node. Might be null.
			 */
			virtual std::unique_ptr<TemporaryNode<T>> getTempTargetNode() = 0;

			virtual ~DistributedEdge() {}
		};
}}}
#endif
