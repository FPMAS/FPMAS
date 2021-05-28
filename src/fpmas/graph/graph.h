#ifndef FPMAS_GRAPH_H
#define FPMAS_GRAPH_H

/** \file src/fpmas/graph/graph.h
 * Graph implementation.
 */

#include "fpmas/api/graph/graph.h"

#include "fpmas/graph/node.h"
#include "fpmas/graph/edge.h"

namespace fpmas { namespace graph {

	/**
	 * api::graph::Graph implementation.
	 */
	template<
		typename NodeType,
		typename EdgeType
	> class Graph : public virtual api::graph::Graph<NodeType, EdgeType> {
			public:
				using typename api::graph::Graph<NodeType, EdgeType>::NodeIdType;
				using typename api::graph::Graph<NodeType, EdgeType>::NodeIdHash;

				using typename api::graph::Graph<NodeType, EdgeType>::EdgeIdType;
				using typename api::graph::Graph<NodeType, EdgeType>::EdgeIdHash;
				
				using typename api::graph::Graph<NodeType, EdgeType>::NodeMap;
				using typename api::graph::Graph<NodeType, EdgeType>::EdgeMap;

			private:
				std::vector<api::utils::Callback<NodeType*>*> insert_node_callbacks;
				std::vector<api::utils::Callback<NodeType*>*> erase_node_callbacks;
				std::vector<api::utils::Callback<EdgeType*>*> insert_edge_callbacks;
				std::vector<api::utils::Callback<EdgeType*>*> erase_edge_callbacks;

				NodeMap nodes;
				EdgeMap edges;

			public:
				/**
				 * Graph default constructor.
				 */
				Graph() {}
				Graph(const Graph<NodeType, EdgeType>&) = delete;
				Graph<NodeType, EdgeType>& operator=(const Graph<NodeType, EdgeType>&) = delete;

				/**
				 * Graph move constructor.
				 */
				Graph(Graph<NodeType, EdgeType>&& graph);
				/**
				 * Graph move assignment operator.
				 */
				Graph<NodeType, EdgeType>& operator=(Graph<NodeType, EdgeType>&&);

				void insert(NodeType*) override;
				void insert(EdgeType*) override;

				void erase(NodeType*) override;
				void erase(EdgeType*) override;

				void addCallOnInsertNode(api::utils::Callback<NodeType*>* callback) override {
					insert_node_callbacks.push_back(callback);
				}

				std::vector<api::utils::Callback<NodeType*>*> onInsertNodeCallbacks() const override {
					return insert_node_callbacks;
				}

				void addCallOnEraseNode(api::utils::Callback<NodeType*>* callback) override {
					erase_node_callbacks.push_back(callback);
				}

				std::vector<api::utils::Callback<NodeType*>*> onEraseNodeCallbacks() const override {
					return erase_node_callbacks;
				}

				void addCallOnInsertEdge(api::utils::Callback<EdgeType*>* callback) override {
					insert_edge_callbacks.push_back(callback);
				}

				std::vector<api::utils::Callback<EdgeType*>*> onInsertEdgeCallbacks() const override {
					return insert_edge_callbacks;
				}

				void addCallOnEraseEdge(api::utils::Callback<EdgeType*>* callback) override {
					erase_edge_callbacks.push_back(callback);
				}

				std::vector<api::utils::Callback<EdgeType*>*> onEraseEdgeCallbacks() const override {
					return erase_edge_callbacks;
				}

				// Node getters
				NodeType* getNode(NodeIdType) override;
				const NodeType* getNode(NodeIdType) const override;
				const NodeMap& getNodes() const override;

				// Edge getters
				EdgeType* getEdge(EdgeIdType) override;
				const EdgeType* getEdge(EdgeIdType) const override;
				const EdgeMap& getEdges() const override;

				void clear() override;

				virtual ~Graph();
	};

	template<typename NodeType, typename EdgeType>
		Graph<NodeType, EdgeType>::Graph(Graph<NodeType, EdgeType>&& graph) {
			this->nodes = std::move(graph.nodes);
			this->edges = std::move(graph.edges);
			this->insert_node_callbacks = std::move(graph.insert_node_callbacks);
			this->insert_edge_callbacks = std::move(graph.insert_edge_callbacks);
			this->erase_node_callbacks = std::move(graph.erase_node_callbacks);
			this->erase_edge_callbacks = std::move(graph.erase_edge_callbacks);
		}

	template<typename NodeType, typename EdgeType>
		Graph<NodeType, EdgeType>& Graph<NodeType, EdgeType>::operator=(Graph<NodeType, EdgeType>&& graph) {
			this->clear();
			for(auto callback : insert_node_callbacks)
				delete callback;
			for(auto callback : erase_node_callbacks)
				delete callback;
			for(auto callback : insert_edge_callbacks)
				delete callback;
			for(auto callback : erase_edge_callbacks)
				delete callback;

			this->nodes = std::move(graph.nodes);
			this->edges = std::move(graph.edges);
			this->insert_node_callbacks = std::move(graph.insert_node_callbacks);
			this->insert_edge_callbacks = std::move(graph.insert_edge_callbacks);
			this->erase_node_callbacks = std::move(graph.erase_node_callbacks);
			this->erase_edge_callbacks = std::move(graph.erase_edge_callbacks);

			return *this;
		}


	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::insert(NodeType* node) {
			FPMAS_LOGD(-1, "GRAPH", "Insert node %s", FPMAS_C_STR(node->getId()));
			this->nodes.insert({node->getId(), node});
			for(auto callback : insert_node_callbacks)
				callback->call(node);
		}

	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::insert(EdgeType* edge) {
			FPMAS_LOGD(
					-1, "GRAPH", "Insert edge %s-{%i} (%p) (from %s to %s)",
					FPMAS_C_STR(edge->getId()), edge->getLayer(), edge,
					FPMAS_C_STR(edge->getSourceNode()->getId()),
					FPMAS_C_STR(edge->getTargetNode()->getId())
					);
			this->edges.insert({edge->getId(), edge});
			for(auto callback : insert_edge_callbacks)
				callback->call(edge);
		}

	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::erase(NodeType* node) {
			FPMAS_LOGD(-1, "GRAPH", "Erase node %s", FPMAS_C_STR(node->getId()));
			// Deletes incoming edges
			for(auto* edge : node->getIncomingEdges()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming edge %s",
						FPMAS_C_STR(edge->getId())
						);
				this->erase(edge);
			}
			// Deletes outgoing edges
			for(auto* edge : node->getOutgoingEdges()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming edge %s",
						FPMAS_C_STR(edge->getId())
						);
				this->erase(edge);
			}

			auto id = node->getId();
			// Removes the node from the global nodes index
			nodes.erase(id);
			// Triggers callbacks
			for(auto callback : erase_node_callbacks)
				callback->call(node);

			// Deletes the node
			delete node;
			FPMAS_LOGD(-1, "GRAPH", "Node %s removed.", FPMAS_C_STR(id));
		}

	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::erase(EdgeType* edge) {
			auto id = edge->getId();
			FPMAS_LOGD(
					-1, "GRAPH", "Erase edge %s-{%i} (%p) (from %s to %s)",
					FPMAS_C_STR(id), edge->getLayer(), edge,
					FPMAS_C_STR(edge->getSourceNode()->getId()),
					FPMAS_C_STR(edge->getTargetNode()->getId())
					);
			// Removes the incoming edges from the incoming/outgoing
			// edge lists of target/source nodes.
			edge->getSourceNode()->unlinkOut(edge);
			edge->getTargetNode()->unlinkIn(edge);

			// Erases edge from the graph
			this->edges.erase(id);
			// Triggers callbacks
			for(auto callback : erase_edge_callbacks)
				callback->call(edge);

			// Deletes edge
			delete edge;
			FPMAS_LOGD(-1, "GRAPH", "Edge %s removed.", FPMAS_C_STR(id));
		}

	template<typename NodeType, typename EdgeType>
		NodeType*
			Graph<NodeType, EdgeType>::getNode(NodeIdType id) {
				return this->nodes.at(id);
		}

	template<typename NodeType, typename EdgeType>
		const NodeType*
			Graph<NodeType, EdgeType>::getNode(NodeIdType id) const {
				return this->nodes.at(id);
		}

	template<typename NodeType, typename EdgeType>
		const typename Graph<NodeType, EdgeType>::NodeMap&
			Graph<NodeType, EdgeType>::getNodes() const {
				return this->nodes;
			}

	template<typename NodeType, typename EdgeType>
		EdgeType*
			Graph<NodeType, EdgeType>::getEdge(EdgeIdType id) {
				return this->edges.at(id);
		}

	template<typename NodeType, typename EdgeType>
		const EdgeType*
			Graph<NodeType, EdgeType>::getEdge(EdgeIdType id) const {
				return this->edges.at(id);
		}

	template<typename NodeType, typename EdgeType>
		const typename Graph<NodeType, EdgeType>::EdgeMap&
			Graph<NodeType, EdgeType>::getEdges() const {
				return this->edges;
			}

	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::clear() {
			// This is VERY hacky.
			std::vector<EdgeType*> _edges;
			for(auto edge : this->edges)
				_edges.push_back(edge.second);

			for(auto edge : _edges) {
				erase(edge);
			}

			std::vector<NodeType*> _nodes;
			for(auto node : this->nodes)
				_nodes.push_back(node.second);

			for(auto node : _nodes) {
				erase(node);
			}
		}

	template<typename NodeType, typename EdgeType>
		Graph<NodeType, EdgeType>::~Graph() {
			clear();
			for(auto callback : insert_node_callbacks)
				delete callback;
			for(auto callback : erase_node_callbacks)
				delete callback;
			for(auto callback : insert_edge_callbacks)
				delete callback;
			for(auto callback : erase_edge_callbacks)
				delete callback;
		}

}}
#endif
