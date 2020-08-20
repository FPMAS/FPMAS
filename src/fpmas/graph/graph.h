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
				void insert(NodeType*) override;
				void insert(EdgeType*) override;

				void erase(NodeType*) override;
				void erase(EdgeType*) override;

				void addCallOnInsertNode(api::utils::Callback<NodeType*>* callback) override {
					insert_node_callbacks.push_back(callback);
				}
				void addCallOnEraseNode(api::utils::Callback<NodeType*>* callback) override {
					erase_node_callbacks.push_back(callback);
				}

				void addCallOnInsertEdge(api::utils::Callback<EdgeType*>* callback) override {
					insert_edge_callbacks.push_back(callback);
				}
				void addCallOnEraseEdge(api::utils::Callback<EdgeType*>* callback) override {
					erase_edge_callbacks.push_back(callback);
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
		void Graph<NodeType, EdgeType>::insert(NodeType* node) {
			FPMAS_LOGD(-1, "GRAPH", "Insert node %s", FPMAS_C_STR(node->getId()));
			this->nodes.insert({node->getId(), node});
			for(auto callback : insert_node_callbacks)
				callback->call(node);
		}

	template<typename NodeType, typename EdgeType>
		void Graph<NodeType, EdgeType>::insert(EdgeType* edge) {
			FPMAS_LOGD(
					-1, "GRAPH", "Insert edge %s (%p) (from %s to %s)",
					FPMAS_C_STR(edge->getId()), edge,
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
					-1, "GRAPH", "Erase edge %s (%p) (from %s to %s)",
					FPMAS_C_STR(id), edge,
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
			std::vector<EdgeType*> edges;
			for(auto edge : this->edges)
				edges.push_back(edge.second);

			for(auto edge : edges) {
				erase(edge);
			}

			std::vector<NodeType*> nodes;
			for(auto node : this->nodes)
				nodes.push_back(node.second);

			for(auto node : nodes) {
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
