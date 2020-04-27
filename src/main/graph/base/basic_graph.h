#ifndef BASIC_GRAPH_H
#define BASIC_GRAPH_H

#include "utils/log.h"
#include "utils/macros.h"
#include "api/graph/base/graph.h"

#include "basic_node.h"
#include "basic_arc.h"

namespace FPMAS::graph::base {

	template<
		typename NodeType,
		typename ArcType
	> class AbstractGraphBase : public virtual FPMAS::api::graph::base::
		Graph<NodeType, ArcType> {
			public:
				typedef FPMAS::api::graph::base::Graph<NodeType, ArcType> graph_base;
				using typename graph_base::node_type;
				using typename graph_base::node_base;
				using typename graph_base::arc_type;
				using typename graph_base::arc_base;

				using typename graph_base::node_id_type;
				using typename graph_base::node_id_hash;

				using typename graph_base::arc_id_type;
				using typename graph_base::arc_id_hash;
				
				using typename graph_base::node_map;
				using typename graph_base::arc_map;

			private:
				node_map nodes;
				arc_map arcs;

			protected:
				node_id_type nodeId;
				arc_id_type arcId;

			public:
				void insert(node_type*) override;
				void insert(arc_type*) override;

				void erase(node_base*) override;
				void erase(arc_base*) override;

				// Node getters
				const node_id_type& currentNodeId() const override {return nodeId;}
				node_type* getNode(node_id_type) override;
				const node_type* getNode(node_id_type) const override;
				const node_map& getNodes() const override;

				// Arc getters
				const node_id_type& currentArcId() const override {return arcId;}
				arc_type* getArc(arc_id_type) override;
				const arc_type* getArc(arc_id_type) const override;
				const arc_map& getArcs() const override;

				~AbstractGraphBase();
	};

	template<GRAPH_PARAMS>
		void AbstractGraphBase<GRAPH_PARAMS_SPEC>::insert(node_type* node) {
			this->nodes.insert({node->getId(), node});
		}

	template<GRAPH_PARAMS>
		void AbstractGraphBase<GRAPH_PARAMS_SPEC>::insert(arc_type* arc) {
			this->arcs.insert({arc->getId(), arc});
		}

	template<GRAPH_PARAMS>
		void AbstractGraphBase<GRAPH_PARAMS_SPEC>::erase(node_base* node) {
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
			// Deletes the node
			delete node;
			FPMAS_LOGD(-1, "GRAPH", "Node %s removed.", ID_C_STR(id));
		}

	template<GRAPH_PARAMS>
		void AbstractGraphBase<GRAPH_PARAMS_SPEC>::erase(arc_base* arc) {
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

			this->arcs.erase(id);
			delete arc;
			FPMAS_LOGD(-1, "GRAPH", "Arc %s removed.", ID_C_STR(id));
		}

	template<GRAPH_PARAMS>
		typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::node_type*
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getNode(node_id_type id) {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::node_type*
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getNode(node_id_type id) const {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::node_map&
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getNodes() const {
				return this->nodes;
			}

	template<GRAPH_PARAMS>
		typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::arc_type*
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getArc(arc_id_type id) {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::arc_type*
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getArc(arc_id_type id) const {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const typename AbstractGraphBase<GRAPH_PARAMS_SPEC>::arc_map&
			AbstractGraphBase<GRAPH_PARAMS_SPEC>::getArcs() const {
				return this->arcs;
			}

	template<GRAPH_PARAMS>
		AbstractGraphBase<GRAPH_PARAMS_SPEC>::~AbstractGraphBase() {
			for(auto node : this->nodes) {
				delete node.second;
			}
			for(auto arc : this->arcs) {
				delete arc.second;
			}
		}

}
#endif
