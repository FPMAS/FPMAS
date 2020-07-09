#ifndef BASIC_GRAPH_H
#define BASIC_GRAPH_H

#include "fpmas/utils/log.h"
#include "fpmas/utils/macros.h"
#include "fpmas/api/graph/base/graph.h"

#include "node.h"
#include "edge.h"

namespace fpmas::graph::base {

	template<
		typename NodeType,
		typename EdgeType
	> class Graph : public virtual api::graph::base::Graph<NodeType, EdgeType> {
			public:
				typedef fpmas::api::graph::base::Graph<NodeType, EdgeType> GraphBase;
				using typename GraphBase::NodeBase;
				using typename GraphBase::EdgeBase;

				using typename GraphBase::NodeIdType;
				using typename GraphBase::NodeIdHash;

				using typename GraphBase::EdgeIdType;
				using typename GraphBase::EdgeIdHash;
				
				using typename GraphBase::NodeMap;
				using typename GraphBase::EdgeMap;

			private:
				std::vector<api::utils::Callback<NodeType*>*> insert_node_callbacks;
				std::vector<api::utils::Callback<NodeType*>*> erase_node_callbacks;
				std::vector<api::utils::Callback<EdgeType*>*> insert_edge_callbacks;
				std::vector<api::utils::Callback<EdgeType*>*> erase_edge_callbacks;

				NodeMap nodes;
				EdgeMap edges;

			protected:
				NodeIdType node_id;
				EdgeIdType edge_id;

			public:
				void insert(NodeType*) override;
				void insert(EdgeType*) override;

				void erase(NodeBase*) override;
				void erase(EdgeBase*) override;

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
				const NodeIdType& currentNodeId() const override {return node_id;}
				NodeType* getNode(NodeIdType) override;
				const NodeType* getNode(NodeIdType) const override;
				const NodeMap& getNodes() const override;

				// Edge getters
				const EdgeIdType& currentEdgeId() const override {return edge_id;}
				EdgeType* getEdge(EdgeIdType) override;
				const EdgeType* getEdge(EdgeIdType) const override;
				const EdgeMap& getEdges() const override;

				void clear() override;

				virtual ~Graph();
	};

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::insert(NodeType* node) {
			FPMAS_LOGD(-1, "GRAPH", "Insert node %s", ID_C_STR(node->getId()));
			this->nodes.insert({node->getId(), node});
			for(auto callback : insert_node_callbacks)
				callback->call(node);
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::insert(EdgeType* edge) {
			FPMAS_LOGD(
					-1, "GRAPH", "Insert edge %s (%p) (from %s to %s)",
					ID_C_STR(edge->getId()), edge,
					ID_C_STR(edge->getSourceNode()->getId()),
					ID_C_STR(edge->getTargetNode()->getId())
					);
			this->edges.insert({edge->getId(), edge});
			for(auto callback : insert_edge_callbacks)
				callback->call(edge);
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::erase(NodeBase* node) {
			FPMAS_LOGD(-1, "GRAPH", "Erase node %s", ID_C_STR(node->getId()));
			// Deletes incoming edges
			for(auto* edge : node->getIncomingEdges()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming edge %s",
						ID_C_STR(edge->getId())
						);
				this->erase(edge);
			}
			// Deletes outgoing edges
			for(auto* edge : node->getOutgoingEdges()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming edge %s",
						ID_C_STR(edge->getId())
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
			FPMAS_LOGD(-1, "GRAPH", "Node %s removed.", ID_C_STR(id));
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::erase(EdgeBase* edge) {
			auto id = edge->getId();
			FPMAS_LOGD(
					-1, "GRAPH", "Erase edge %s (%p) (from %s to %s)",
					ID_C_STR(id), edge,
					ID_C_STR(edge->getSourceNode()->getId()),
					ID_C_STR(edge->getTargetNode()->getId())
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
			FPMAS_LOGD(-1, "GRAPH", "Edge %s removed.", ID_C_STR(id));
		}

	template<GRAPH_PARAMS>
		NodeType*
			Graph<GRAPH_PARAMS_SPEC>::getNode(NodeIdType id) {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const NodeType*
			Graph<GRAPH_PARAMS_SPEC>::getNode(NodeIdType id) const {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const typename Graph<GRAPH_PARAMS_SPEC>::NodeMap&
			Graph<GRAPH_PARAMS_SPEC>::getNodes() const {
				return this->nodes;
			}

	template<GRAPH_PARAMS>
		EdgeType*
			Graph<GRAPH_PARAMS_SPEC>::getEdge(EdgeIdType id) {
				return this->edges.at(id);
		}

	template<GRAPH_PARAMS>
		const EdgeType*
			Graph<GRAPH_PARAMS_SPEC>::getEdge(EdgeIdType id) const {
				return this->edges.at(id);
		}

	template<GRAPH_PARAMS>
		const typename Graph<GRAPH_PARAMS_SPEC>::EdgeMap&
			Graph<GRAPH_PARAMS_SPEC>::getEdges() const {
				return this->edges;
			}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::clear() {
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

	template<GRAPH_PARAMS>
		Graph<GRAPH_PARAMS_SPEC>::~Graph() {
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

}
#endif
