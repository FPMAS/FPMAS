#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>
#include <array>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<typename, typename, int> class Node;
	template<typename, typename, int> class Graph;
	template<typename, typename, int> class Arc;
	template<typename, typename, int> class Layer;

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
	template<typename T, typename IdType, int N>
	class Node : public GraphItem<IdType> {
		friend Node<T, IdType, N> nlohmann::adl_serializer<Node<T, IdType, N>>::from_json(const json&);
		friend Arc<T, IdType, N> nlohmann::adl_serializer<Arc<T, IdType, N>>::from_json(const json&);
		// Grants access to incoming and outgoing arcs lists
		friend Arc<T, IdType, N>::Arc(IdType, Node<T, IdType, N>*, Node<T, IdType, N>*, LayerId);
		friend void Arc<T, IdType, N>::unlink();
		// Grants access to private Node constructor
		friend Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType);
		friend Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType id, T&& data);
		friend Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType id, float weight, T&& data);

		// Allows graph to delete arcs from incoming / outgoing arcs lists
		friend void Graph<T, IdType, N>::removeNode(IdType nodeId);

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
		std::array<Layer<T, IdType, N>, N> layers;
		Layer<T, IdType, N>& layer(LayerId layer);

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
		const Layer<T, IdType, N>& layer(LayerId layer) const;

		/**
		 * Returns a const reference to the complete array of layers associated to
		 * this node.
		 *
		 * @return const reference to this node's layers std::array
		 */
		const std::array<Layer<T, IdType, N>, N>& getLayers() const;

		/**
		 * Returns a vector containing pointers to this node's incoming arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to incoming arcs
		 */
		std::vector<Arc<T, IdType, N>*> getIncomingArcs() const;
		/**
		 * Returns the incoming neighbors list of this node, computed as source
		 * nodes of each incoming arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return incoming neighbors list
		 */
		std::vector<Node<T, IdType, N>*> inNeighbors() const;
		/**
		 * Returns a vector containing pointers to this node's outgoing arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to outgoing arcs
		 */
		std::vector<Arc<T, IdType, N>*> getOutgoingArcs() const;
		/**
		 * Returns the outgoing neighbors list of this node, computed as target
		 * nodes of each outgoing arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return outgoing neighbors list
		 */
		std::vector<Node<T, IdType, N>*> outNeighbors() const;

	};

	template<typename T, typename IdType, int N> Node<T, IdType, N>::Node(IdType id) : Node<T, IdType, N>(id, 1., T()) {
	}

	template<typename T, typename IdType, int N> Node<T, IdType, N>::Node(IdType id, T&& data) : Node<T, IdType, N>(id, 1., std::forward<T>(data)) {
	}

	template<typename T, typename IdType, int N> Node<T, IdType, N>::Node(IdType id, float weight, T&& data) : GraphItem<IdType>(id), _data(std::forward<T>(data)), weight(weight) {
		for (int i = 0; i < N; i++) {
			this->layers[i] = Layer<T, IdType, N>();
		}
	}

	template<typename T, typename IdType, int N> T& Node<T, IdType, N>::data() {
		return this->_data;
	}

	template<typename T, typename IdType, int N> const T& Node<T, IdType, N>::data() const {
		return this->_data;
	}

	template<typename T, typename IdType, int N> float Node<T, IdType, N>::getWeight() const {
		return this->weight;
	}

	template<typename T, typename IdType, int N> void Node<T, IdType, N>::setWeight(float weight) {
		this->weight = weight;
	}

	template<typename T, typename IdType, int N> Layer<T, IdType, N>& Node<T, IdType, N>::layer(LayerId layer) {
		return this->layers[layer];
	}

	template<typename T, typename IdType, int N> const Layer<T, IdType, N>& Node<T, IdType, N>::layer(LayerId layer) const {
		return this->layers[layer];
	}

	template<typename T, typename IdType, int N> const std::array<Layer<T, IdType, N>, N>& Node<T, IdType, N>::getLayers() const {
		return this->layers;
	}

	template<typename T, typename IdType, int N> std::vector<Arc<T, IdType, N>*> Node<T, IdType, N>::getIncomingArcs() const {
		return this->layers[0].getIncomingArcs();
	}


	template<typename T, typename IdType, int N> std::vector<Node<T, IdType, N>*> Node<T, IdType, N>::inNeighbors() const {
		std::vector<Node<T, IdType, N>*> neighbors;
		for(auto arc : this->layers[0].getIncomingArcs())
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	template<typename T, typename IdType, int N> std::vector<Arc<T, IdType, N>*> Node<T, IdType, N>::getOutgoingArcs() const {
		return this->layers[0].getOutgoingArcs();
	}

	template<typename T, typename IdType, int N> std::vector<Node<T, IdType, N>*> Node<T, IdType, N>::outNeighbors() const {
		std::vector<Node<T, IdType, N>*> neighbors;
		for(auto arc : this->layers[0].getOutgoingArcs())
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}

#endif
