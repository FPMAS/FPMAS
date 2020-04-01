#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>
#include <array>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<typename, typename> class Node;
	template<typename, typename> class Graph;
	template<typename, typename> class Arc;
	template<typename, typename> class Layer;

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
	template<typename T, typename IdType>
	class Node : public GraphItem<IdType> {
		friend Node<T, IdType> nlohmann::adl_serializer<Node<T, IdType>>::from_json(const json&);
		friend Arc<T, IdType> nlohmann::adl_serializer<Arc<T, IdType>>::from_json(const json&);
		// Grants access to incoming and outgoing arcs lists
		friend Arc<T, IdType>::Arc(IdType, Node<T, IdType>*, Node<T, IdType>*, LayerId);
		friend void Arc<T, IdType>::unlink();
		// Grants access to private Node constructor
		friend Node<T, IdType>* Graph<T, IdType>::buildNode();
		friend Node<T, IdType>* Graph<T, IdType>::buildNode(T&&);
		friend Node<T, IdType>* Graph<T, IdType>::buildNode(float, T&&);
		friend Node<T, IdType>* Graph<T, IdType>::_buildNode(IdType, float, T&&);

		// Allows graph to delete arcs from incoming / outgoing arcs lists
		friend void Graph<T, IdType>::removeNode(IdType);

		protected:

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 */
		Node(IdType id);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 */
		Node(IdType id, T&& data);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data
		 */
		Node(IdType id, float weight, T&& data);

		private:
		T _data;
		float weight;
		mutable std::unordered_map<LayerId, Layer<T, IdType>> layers;
		Layer<T, IdType>& _layer(LayerId layer);

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
		const Layer<T, IdType>& layer(LayerId layer) const;

		/**
		 * Returns a const reference to the complete array of layers associated to
		 * this node.
		 *
		 * @return const reference to this node's layers std::array
		 */
		const std::unordered_map<LayerId, Layer<T, IdType>>& getLayers() const;

		/**
		 * Returns a vector containing pointers to this node's incoming arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to incoming arcs
		 */
		std::vector<Arc<T, IdType>*> getIncomingArcs() const;
		/**
		 * Returns the incoming neighbors list of this node, computed as source
		 * nodes of each incoming arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return incoming neighbors list
		 */
		std::vector<Node<T, IdType>*> inNeighbors() const;
		/**
		 * Returns a vector containing pointers to this node's outgoing arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to outgoing arcs
		 */
		std::vector<Arc<T, IdType>*> getOutgoingArcs() const;
		/**
		 * Returns the outgoing neighbors list of this node, computed as target
		 * nodes of each outgoing arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return outgoing neighbors list
		 */
		std::vector<Node<T, IdType>*> outNeighbors() const;

	};

	template<typename T, typename IdType> Node<T, IdType>::Node(IdType id) : Node<T, IdType>(id, 1., T()) {
	}

	template<typename T, typename IdType> Node<T, IdType>::Node(IdType id, T&& data) : Node<T, IdType>(id, 1., std::forward<T>(data)) {
	}

	template<typename T, typename IdType> Node<T, IdType>::Node(IdType id, float weight, T&& data)
		: GraphItem<IdType>(id), _data(std::forward<T>(data)), weight(weight) {
	}

	template<typename T, typename IdType> T& Node<T, IdType>::data() {
		return this->_data;
	}

	template<typename T, typename IdType> const T& Node<T, IdType>::data() const {
		return this->_data;
	}

	template<typename T, typename IdType> float Node<T, IdType>::getWeight() const {
		return this->weight;
	}

	template<typename T, typename IdType> void Node<T, IdType>::setWeight(float weight) {
		this->weight = weight;
	}

	template<typename T, typename IdType> Layer<T, IdType>& Node<T, IdType>::_layer(LayerId layer) {
		return this->layers[layer];
	}

	template<typename T, typename IdType> const Layer<T, IdType>& Node<T, IdType>::layer(LayerId layer) const {
		return this->layers[layer];
	}

	template<typename T, typename IdType> const std::unordered_map<LayerId, Layer<T, IdType>>& Node<T, IdType>::getLayers() const {
		return this->layers;
	}

	template<typename T, typename IdType> std::vector<Arc<T, IdType>*> Node<T, IdType>::getIncomingArcs() const {
		return this->layers[0].getIncomingArcs();
	}


	template<typename T, typename IdType> std::vector<Node<T, IdType>*> Node<T, IdType>::inNeighbors() const {
		std::vector<Node<T, IdType>*> neighbors;
		for(auto arc : this->layers[0].getIncomingArcs())
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	template<typename T, typename IdType> std::vector<Arc<T, IdType>*> Node<T, IdType>::getOutgoingArcs() const {
		return this->layers[0].getOutgoingArcs();
	}

	template<typename T, typename IdType> std::vector<Node<T, IdType>*> Node<T, IdType>::outNeighbors() const {
		std::vector<Node<T, IdType>*> neighbors;
		for(auto arc : this->layers[0].getOutgoingArcs())
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}

#endif
