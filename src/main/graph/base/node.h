#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>
#include <array>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"
#include "fossil.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<NODE_PARAMS> class Node;
	template<NODE_PARAMS> class Graph;
	template<NODE_PARAMS> class Arc;
	template<NODE_PARAMS> class Layer;

	/**
	 * Graph node.
	 *
	 * Each Node contains :
	 * - an instance of `T`
	 * - a weight
	 * - `incoming` and `outgoing` arcs lists organized by Layer
	 *
	 * A Layer is managed by the Node for each layer type defined in the
	 * `LayerType` enum, for a total of `N` layers.
	 *
	 * @tparam T associated data type
	 * @tparam LayerType enum describing the available layers
	 * @tparam N number of layers
	 */
	template<NODE_PARAMS>
	class Node : public GraphItem {
		friend NODE nlohmann::adl_serializer<NODE>::from_json(const json&);
		friend ARC nlohmann::adl_serializer<ARC>::from_json(const json&);
		// Grants access to incoming and outgoing arcs lists
		friend ARC::Arc(unsigned long, NODE*, NODE*, LayerId);
		// Grants access to private Node constructor
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long);
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, T&& data);
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, float weight, T&& data);
		// Grants access to layers
		friend FossilArcs<ARC> Graph<NODE_PARAMS_SPEC>::removeNode(unsigned long);
		// Allows removeNode function to remove deleted arcs
		friend bool Graph<NODE_PARAMS_SPEC>::unlink(ARC*);

		protected:

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 */
		Node(unsigned long id);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 */
		Node(unsigned long id, T&& data);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data
		 */
		Node(unsigned long id, float weight, T&& data);

		private:
		T _data;
		float weight;
		std::array<Layer<NODE_PARAMS_SPEC>, N> layers;
		Layer<NODE_PARAMS_SPEC>& layer(LayerId layer);

		public:
		/**
		 * Reference to node's data.
		 *
		 * @return reference to node's data
		 */
		T& data();
		/**
		 * Const reference to node's data.
		 *
		 * @return const reference to node's data
		 */
		const T& data() const;

		/**
		 * Returns node's weight
		 *
		 * Default value is 1., if weight has not been provided by the
		 * user. This weight might for example be used by graph partitionning
		 * functions to distribute the graph.
		 *
		 * @return node's weight
		 */
		float getWeight() const;

		/**
		 * Sets node's weight to the specified value.
		 *
		 * @param weight
		 */
		void setWeight(float weight);

		/**
		 * Returns a const reference to the specified layer of this node.
		 *
		 * @return const reference to layer
		 */
		const Layer<NODE_PARAMS_SPEC>& layer(LayerId layer) const;

		/**
		 * Returns a const reference to the complete array of layers associated to
		 * this node.
		 *
		 * @return const reference to this node's layers std::array
		 */
		const std::array<Layer<NODE_PARAMS_SPEC>, N>& getLayers() const;

		/**
		 * Returns a vector containing pointers to this node's incoming arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to incoming arcs
		 */
		std::vector<ARC*> getIncomingArcs() const;
		/**
		 * Returns the incoming neighbors list of this node, computed as source
		 * nodes of each incoming arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return incoming neighbors list
		 */
		std::vector<NODE*> inNeighbors() const;
		/**
		 * Returns a vector containing pointers to this node's outgoing arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to outgoing arcs
		 */
		std::vector<ARC*> getOutgoingArcs() const;
		/**
		 * Returns the outgoing neighbors list of this node, computed as target
		 * nodes of each outgoing arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return outgoing neighbors list
		 */
		std::vector<NODE*> outNeighbors() const;

	};

	template<NODE_PARAMS> Node<NODE_PARAMS_SPEC>::Node(unsigned long id) : NODE(id, 1., T()) {
	}

	template<NODE_PARAMS> Node<NODE_PARAMS_SPEC>::Node(unsigned long id, T&& data) : NODE(id, 1., std::move(data)) {
	}

	template<NODE_PARAMS> Node<NODE_PARAMS_SPEC>::Node(unsigned long id, float weight, T&& data) : GraphItem(id), _data(std::move(data)), weight(weight) {
		for (int i = 0; i < N; i++) {
			this->layers[i] = Layer<NODE_PARAMS_SPEC>();
		}
	}

	template<NODE_PARAMS> T& Node<NODE_PARAMS_SPEC>::data() {
		return this->_data;
	}

	template<NODE_PARAMS> const T& Node<NODE_PARAMS_SPEC>::data() const {
		return this->_data;
	}

	template<NODE_PARAMS> float Node<NODE_PARAMS_SPEC>::getWeight() const {
		return this->weight;
	}

	template<NODE_PARAMS> void Node<NODE_PARAMS_SPEC>::setWeight(float weight) {
		this->weight = weight;
	}

	template<NODE_PARAMS> Layer<NODE_PARAMS_SPEC>& Node<NODE_PARAMS_SPEC>::layer(LayerId layer) {
		return this->layers[layer];
	}

	template<NODE_PARAMS> const Layer<NODE_PARAMS_SPEC>& Node<NODE_PARAMS_SPEC>::layer(LayerId layer) const {
		return this->layers[layer];
	}

	template<NODE_PARAMS> const std::array<Layer<NODE_PARAMS_SPEC>, N>& Node<NODE_PARAMS_SPEC>::getLayers() const {
		return this->layers;
	}

	template<NODE_PARAMS> std::vector<ARC*> Node<NODE_PARAMS_SPEC>::getIncomingArcs() const {
		return this->layers[0].getIncomingArcs();
	}


	template<NODE_PARAMS> std::vector<NODE*> Node<NODE_PARAMS_SPEC>::inNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->layers[0].getIncomingArcs())
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	template<NODE_PARAMS> std::vector<ARC*> Node<NODE_PARAMS_SPEC>::getOutgoingArcs() const {
		return this->layers[0].getOutgoingArcs();
	}

	template<NODE_PARAMS> std::vector<NODE*> Node<NODE_PARAMS_SPEC>::outNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->layers[0].getOutgoingArcs())
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}

#endif
