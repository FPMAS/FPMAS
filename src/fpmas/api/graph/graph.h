#ifndef FPMAS_GRAPH_API_H
#define FPMAS_GRAPH_API_H

/** \file src/fpmas/api/graph/graph.h
 * Graph API
 */

#include <unordered_map>
#include "fpmas/api/graph/node.h"
#include "fpmas/api/graph/id.h"
#include "fpmas/api/utils/callback.h"

namespace fpmas { namespace api { namespace graph {
		/**
		 * Graph API
		 *
		 * @tparam NodeType Node implementation
		 * @tparam EdgeType Edge implementation
		 */
		template<typename NodeType, typename EdgeType>
			class Graph {
				static_assert(std::is_base_of<typename NodeType::EdgeType, EdgeType>::value,
						"NodeType::EdgeType must be a base of EdgeType.");
				static_assert(std::is_base_of<typename EdgeType::NodeType, NodeType>::value,
						"EdgeType::NodeType must be a base of NodeType.");
				public:
				/**
				 * Node ID type
				 */
				typedef typename NodeType::IdType NodeIdType;
				/**
				 * Edge ID type
				 */
				typedef typename EdgeType::IdType EdgeIdType;

				/**
				 * Node ID hash function
				 */
				typedef fpmas::api::graph::IdHash<typename NodeType::IdType> NodeIdHash;
				/**
				 * Edge ID hash function
				 */
				typedef fpmas::api::graph::IdHash<typename EdgeType::IdType> EdgeIdHash;

				/**
				 * Node map type
				 */
				typedef std::unordered_map<
					NodeIdType, NodeType*, NodeIdHash
					> NodeMap;
				/**
				 * Edge map type
				 */
				typedef std::unordered_map<
					EdgeIdType, EdgeType*, EdgeIdHash
					> EdgeMap;

				public:
				/**
				 * Inserts a Node into the graph.
				 *
				 * Upon return, the NodeMap returned by getNodes() must
				 * contain an entry `{node->getId(), node}`.
				 *
				 * Callbacks registered using addCallOnInsertNode() are also
				 * called after the node has effectively been inserted into the
				 * graph.
				 *
				 * @param node pointer to node to insert in the graph
				 */
				virtual void insert(NodeType* node) = 0;

				/**
				 * Inserts an Edge into the graph.
				 *
				 * Upon return, the EdgeMap returned by getEdges() must
				 * contain an entry `{edge->getId(), edge}`.
				 *
				 * Callbacks registered using addCallOnInsertEdge() are also
				 * called after the edge has effectively been inserted into the
				 * graph.
				 *
				 * @param edge pointer to edge to insert in the graph
				 */
				virtual void insert(EdgeType* edge) = 0;

				
				/**
				 * Erases a previously inserted node from the graph.
				 *
				 * Upon return, the entry `{node->getId(), node}` must **not**
				 * be contained anymore in the NodeMap returned by getNodes().
				 *
				 * Moreover, all incoming and outgoing edges, as returned by
				 * Node::getIncomingEdges and Node::getOutgoingEdges, should
				 * also be erased from the Graph, as specified by the
				 * erase(EdgeType*) function.
				 *
				 * Callbacks registered using addCallOnEraseNode() are also
				 * called after the node has effectively been removed from the
				 * graph.
				 *
				 * Erasing a node not contained in the Graph produces an
				 * unexpected behavior.
				 *
				 * @param node node to erase from the graph
				 */
				virtual void erase(NodeType* node) = 0;

				/**
				 * Erases a previously inserted edge from the graph.
				 *
				 * Upon return, the entry `{edge->getId(), edge}` must **not**
				 * be contained anymore in the NodeMap returned by getEdges().
				 *
				 * Callbacks registered using addCallOnEraseEdge() are also
				 * called after the edge has effectively been removed from the
				 * graph.
				 *
				 * Erasing an edge not contained in the Graph produces an
				 * unexpected behavior.
				 *
				 * @param edge edge to erase from the graph
				 */
				virtual void erase(EdgeType* edge) = 0;

				/**
				 * Adds an insert node callback, called on insert(NodeType*).
				 *
				 * @param callback insert node callback
				 */
				virtual void addCallOnInsertNode(api::utils::Callback<NodeType*>* callback) = 0;

				/**
				 * Adds an erase node callback, called on erase(NodeType*).
				 *
				 * @param callback insert node callback
				 */
				virtual void addCallOnEraseNode(api::utils::Callback<NodeType*>* callback) = 0;

				/**
				 * Adds an insert edge callback, called on insert(EdgeType*).
				 *
				 * @param callback insert edge callback
				 */
				virtual void addCallOnInsertEdge(api::utils::Callback<EdgeType*>* callback) = 0;

				/**
				 * Adds an erase edge callback, called on erase(EdgeType*).
				 *
				 * @param callback erase edge callback
				 */
				virtual void addCallOnEraseEdge(api::utils::Callback<EdgeType*>* callback) = 0;

				// Node getters
				/**
				 * Gets the pointer to the node associated to the given id in
				 * the Graph.
				 *
				 * The returned pointer must correspond to the entry `{id,
				 * node}` contained in the NodeMap returned by getNodes().
				 *
				 * If id does not correspond to a node contained in the graph,
				 * behavior is unspecified.
				 *
				 * @param id node id
				 * @return corresponding node in the graph
				 */
				virtual NodeType* getNode(NodeIdType id) = 0;
				/**
				 * \copydoc getNode(NodeIdType)
				 */
				virtual const NodeType* getNode(NodeIdType id) const = 0;
				/**
				 * NodeMap representing the nodes contained in this graph.
				 *
				 * @return graph's nodes
				 */
				virtual const NodeMap& getNodes() const = 0;

				// Edge getters
				/**
				 * Gets the pointer to the edge associated to the given id in
				 * the Graph.
				 *
				 * The returned pointer must correspond to the entry `{id,
				 * edge}` contained in the EdgeMap returned by getEdges().
				 *
				 * If id does not correspond to an edge contained in the graph,
				 * behavior is unspecified.
				 *
				 * @param id edge id
				 * @return corresponding edge in the graph
				 */
				virtual EdgeType* getEdge(EdgeIdType id) = 0;
				/**
				 * /copydoc getEdge(EdgeIdType)
				 */
				virtual const EdgeType* getEdge(EdgeIdType) const = 0;
				/**
				 * EdgeMap representing the edges contained in this graph.
				 *
				 * @return graph's edges
				 */
				virtual const EdgeMap& getEdges() const = 0;

				/**
				 * Erases all nodes and edges contained in the graph.
				 *
				 * All erase node and erase edge callbacks must be called
				 * properly for each item.
				 */
				virtual void clear() = 0;

				virtual ~Graph() {}
			};
	}}
}
#endif
