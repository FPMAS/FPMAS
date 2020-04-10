#ifndef BASIC_GRAPH_H
#define BASIC_GRAPH_H

#include "utils/log.h"
#include "utils/macros.h"
#include "api/graph/base/graph.h"

#include "basic_node.h"
#include "basic_arc.h"

namespace FPMAS::graph::base {
	using FPMAS::api::graph::base::LayerId;
	template<
		typename T,
		typename IdType,
		template<typename, typename> class node_type_template = BasicNode,
		template<typename, typename> class arc_type_template = BasicArc
	> class BasicGraph : public virtual FPMAS::api::graph::base::
		Graph<T, IdType, node_type_template, arc_type_template> {
			public:
				typedef FPMAS::api::graph::base::Graph<T, IdType, node_type_template, arc_type_template> graph_base;
				typedef typename graph_base::node_type node_type;
				typedef FPMAS::api::graph::base::Node<T, IdType>* abstract_node_ptr;
				typedef typename graph_base::arc_type arc_type;
				typedef FPMAS::api::graph::base::Arc<T, IdType>* abstract_arc_ptr;
				typedef node_type* node_ptr;
				typedef arc_type* arc_ptr;
				typedef typename graph_base::id_hash id_hash;
				typedef std::unordered_map<
					IdType, node_type*, id_hash
					> node_map;
				typedef std::unordered_map<
					IdType, arc_type*, id_hash
					> arc_map;

			private:
				node_map nodes;
				arc_map arcs;

			protected:
				/**
				 * Current Node Id.
				 */
				IdType _currentNodeId;
				/**
				 * Current Arc Id.
				 */
				IdType _currentArcId;

			public:
				node_type* buildNode();
				node_type* buildNode(T&& data);
				node_type* buildNode(T&& data, float weight);

				arc_type* link(abstract_node_ptr, abstract_node_ptr, LayerId) override;

				void removeNode(abstract_node_ptr node) override;

				void unlink(abstract_arc_ptr) override;

				const IdType& currentNodeId() const override {return _currentNodeId;};
				const IdType& currentArcId() const override {return _currentArcId;};

				// Node getters
				node_type* getNode(IdType) override;
				const node_type* getNode(IdType) const override;
				const node_map& getNodes() const override;

				// Arc getters
				arc_type* getArc(IdType) override;
				const arc_type* getArc(IdType) const override;
				const arc_map& getArcs() const override;

				~BasicGraph();
	};

	/**
	 * Builds a node with a default weight and no data, adds it to this
	 * graph, and finally returns the built node.
	 *
	 * @return pointer to built node
	 */
	template<GRAPH_PARAMS> typename BasicGraph<GRAPH_PARAMS_SPEC>::node_ptr
		BasicGraph<GRAPH_PARAMS_SPEC>::buildNode() {
			node_ptr node = new node_type(_currentNodeId);
			this->nodes[_currentNodeId] = node;
			++_currentNodeId;
			return node;
		}

	/**
	 * Builds a node with the specified data, adds it to this graph,
	 * and finally returns the built node.
	 *
	 * @param data node's data
	 * @return pointer to built node
	 */
	template<GRAPH_PARAMS> typename BasicGraph<GRAPH_PARAMS_SPEC>::node_ptr
		BasicGraph<GRAPH_PARAMS_SPEC>::buildNode(T&& data) {
		node_ptr node = new node_type(_currentNodeId, std::forward<T>(data));
		this->nodes[_currentNodeId] = node;
		++_currentNodeId;
		return node;
	}

	/**
	 * Builds a node with the specify id, weight and data, adds it to this graph,
	 * and finally returns a pointer to the built node.
	 *
	 * @param weight node's weight
	 * @param data rvalue reference to node's data
	 * @return pointer to build node
	 */
	template<GRAPH_PARAMS> typename BasicGraph<GRAPH_PARAMS_SPEC>::node_ptr
		BasicGraph<GRAPH_PARAMS_SPEC>::buildNode(T&& data, float weight) {
		node_ptr node = new node_type(
				_currentNodeId, std::forward<T>(data), weight
				);
		this->nodes[_currentNodeId] = node;
		++_currentNodeId;
		return node;
	}

	template<GRAPH_PARAMS>
		void BasicGraph<GRAPH_PARAMS_SPEC>::removeNode(abstract_node_ptr node) {
			FPMAS_LOGD(-1, "GRAPH", "Removing node %s", ID_C_STR(node->getId()));
			// Deletes incoming arcs
			for(auto* arc : node->getIncomingArcs()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming arc %s",
						ID_C_STR(arc->getId())
						);
				this->unlink(arc);
			}
			// Deletes outgoing arcs
			for(auto* arc : node->getOutgoingArcs()) {
				FPMAS_LOGV(
						-1,
						"GRAPH",
						"Unlink incoming arc %s",
						ID_C_STR(arc->getId())
						);
				this->unlink(arc);
			}

			auto id = node->getId();
			// Removes the node from the global nodes index
			nodes.erase(id);
			// Deletes the node
			delete node;
			FPMAS_LOGD(-1, "GRAPH", "Node %s removed.", ID_C_STR(id));
		}

	template<GRAPH_PARAMS>
		typename BasicGraph<GRAPH_PARAMS_SPEC>::arc_type* BasicGraph<GRAPH_PARAMS_SPEC>::link(
				abstract_node_ptr source, abstract_node_ptr target, LayerId layer
				) {
				arc_ptr arc = new arc_type(
						_currentArcId, source, target, layer
						);
				this->arcs[_currentArcId] = arc;
				++_currentArcId;
				return arc;
		}

	template<GRAPH_PARAMS>
		void BasicGraph<GRAPH_PARAMS_SPEC>::unlink(abstract_arc_ptr arc) {
			// Removes the incoming arcs from the incoming/outgoing
			// arc lists of target/source nodes.
			arc->unlink();

			this->arcs.erase(arc->getId());
			delete arc;
		}

	template<GRAPH_PARAMS>
		typename BasicGraph<GRAPH_PARAMS_SPEC>::node_type*
			BasicGraph<GRAPH_PARAMS_SPEC>::getNode(IdType id) {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const typename BasicGraph<GRAPH_PARAMS_SPEC>::node_type*
			BasicGraph<GRAPH_PARAMS_SPEC>::getNode(IdType id) const {
				return this->nodes.at(id);
		}

	template<GRAPH_PARAMS>
		const typename BasicGraph<GRAPH_PARAMS_SPEC>::node_map&
			BasicGraph<GRAPH_PARAMS_SPEC>::getNodes() const {
				return this->nodes;
			}

	template<GRAPH_PARAMS>
		typename BasicGraph<GRAPH_PARAMS_SPEC>::arc_type*
			BasicGraph<GRAPH_PARAMS_SPEC>::getArc(IdType id) {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const typename BasicGraph<GRAPH_PARAMS_SPEC>::arc_type*
			BasicGraph<GRAPH_PARAMS_SPEC>::getArc(IdType id) const {
				return this->arcs.at(id);
		}

	template<GRAPH_PARAMS>
		const typename BasicGraph<GRAPH_PARAMS_SPEC>::arc_map&
			BasicGraph<GRAPH_PARAMS_SPEC>::getArcs() const {
				return this->arcs;
			}

	template<GRAPH_PARAMS>
		BasicGraph<GRAPH_PARAMS_SPEC>::~BasicGraph() {
			for(auto node : this->nodes) {
				delete node.second;
			}
			for(auto arc : this->arcs) {
				delete arc.second;
			}
		}

}
#endif
