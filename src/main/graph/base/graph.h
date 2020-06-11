#ifndef BASIC_GRAPH_H
#define BASIC_GRAPH_H

#include "utils/log.h"
#include "utils/macros.h"
#include "api/graph/base/graph.h"

#include "node.h"
#include "arc.h"

namespace FPMAS::graph::base {

	template<
		typename NodeType,
		typename ArcType
	> class Graph : public virtual api::graph::base::Graph<NodeType, ArcType> {
			public:
				typedef FPMAS::api::graph::base::Graph<NodeType, ArcType> GraphBase;
				using typename GraphBase::NodeBase;
				using typename GraphBase::ArcBase;

				using typename GraphBase::NodeIdType;
				using typename GraphBase::NodeIdHash;

				using typename GraphBase::ArcIdType;
				using typename GraphBase::ArcIdHash;
				
				using typename GraphBase::NodeMap;
				using typename GraphBase::ArcMap;

			private:
				std::vector<api::utils::Callback<NodeType*>*> insert_node_callbacks;
				std::vector<api::utils::Callback<NodeType*>*> erase_node_callbacks;
				std::vector<api::utils::Callback<ArcType*>*> insert_arc_callbacks;
				std::vector<api::utils::Callback<ArcType*>*> erase_arc_callbacks;

				NodeMap nodes;
				ArcMap arcs;

			protected:
				NodeIdType node_id;
				ArcIdType arc_id;

			public:
				void insert(NodeType*) override;
				void insert(ArcType*) override;

				void erase(NodeBase*) override;
				void erase(ArcBase*) override;

				void addCallOnInsertNode(api::utils::Callback<NodeType*>* callback) override {
					insert_node_callbacks.push_back(callback);
				}
				void addCallOnEraseNode(api::utils::Callback<NodeType*>* callback) override {
					erase_node_callbacks.push_back(callback);
				}

				void addCallOnInsertArc(api::utils::Callback<ArcType*>* callback) override {
					insert_arc_callbacks.push_back(callback);
				}
				void addCallOnEraseArc(api::utils::Callback<ArcType*>* callback) override {
					erase_arc_callbacks.push_back(callback);
				}

				// Node getters
				const NodeIdType& currentNodeId() const override {return node_id;}
				NodeType* getNode(NodeIdType) override;
				const NodeType* getNode(NodeIdType) const override;
				const NodeMap& getNodes() const override;

				// Arc getters
				const ArcIdType& currentArcId() const override {return arc_id;}
				ArcType* getArc(ArcIdType) override;
				const ArcType* getArc(ArcIdType) const override;
				const ArcMap& getArcs() const override;

				~Graph();
	};

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::insert(NodeType* node) {
			this->nodes.insert({node->getId(), node});
			for(auto callback : insert_node_callbacks)
				callback->call(node);
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::insert(ArcType* arc) {
			this->arcs.insert({arc->getId(), arc});
			for(auto callback : insert_arc_callbacks)
				callback->call(arc);
		}

	template<GRAPH_PARAMS>
		void Graph<GRAPH_PARAMS_SPEC>::erase(NodeBase* node) {
			FPMAS_LOGD(-1, "GRAPH", "Removing node %s", ID_C_STR(node->getId()));
			// Deletes incoming arcs
			for(auto* arc : node->getIncomingArcs()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming arc %s",
						ID_C_STR(arc->getId())
						);
				this->erase(arc);
			}
			// Deletes outgoing arcs
			for(auto* arc : node->getOutgoingArcs()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming arc %s",
						ID_C_STR(arc->getId())
						);
				this->erase(arc);
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
		void Graph<GRAPH_PARAMS_SPEC>::erase(ArcBase* arc) {
			auto id = arc->getId();
			FPMAS_LOGD(
					-1, "GRAPH", "Erase arc %s (%p) (from %s to %s)",
					ID_C_STR(id), arc,
					ID_C_STR(arc->getSourceNode()->getId()),
					ID_C_STR(arc->getTargetNode()->getId())
					);
			// Removes the incoming arcs from the incoming/outgoing
			// arc lists of target/source nodes.
			arc->getSourceNode()->unlinkOut(arc);
			arc->getTargetNode()->unlinkIn(arc);

			// Erases arc from the graph
			this->arcs.erase(id);
			// Triggers callbacks
			for(auto callback : erase_arc_callbacks)
				callback->call(arc);

			// Deletes arc
			delete arc;
			FPMAS_LOGD(-1, "GRAPH", "Arc %s removed.", ID_C_STR(id));
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
		ArcType*
			Graph<GRAPH_PARAMS_SPEC>::getArc(ArcIdType id) {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const ArcType*
			Graph<GRAPH_PARAMS_SPEC>::getArc(ArcIdType id) const {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const typename Graph<GRAPH_PARAMS_SPEC>::ArcMap&
			Graph<GRAPH_PARAMS_SPEC>::getArcs() const {
				return this->arcs;
			}

	template<GRAPH_PARAMS>
		Graph<GRAPH_PARAMS_SPEC>::~Graph() {
			for(auto node : this->nodes) {
				delete node.second;
			}
			for(auto arc : this->arcs) {
				delete arc.second;
			}
		}

}
#endif
