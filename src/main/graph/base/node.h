#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>
#include <array>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<typename T, int N> class Node;
	template<typename T, int N> class Graph;
	template<typename T, int N> class Arc;
	template<typename T, int N> class Layer;

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
	template<typename T, int N>
	class Node : public GraphItem<NodeId> {
		friend Node<T, N> nlohmann::adl_serializer<Node<T, N>>::from_json(const json&);
		friend Arc<T, N> nlohmann::adl_serializer<Arc<T, N>>::from_json(const json&);
		// Grants access to incoming and outgoing arcs lists
		friend Arc<T, N>::Arc(ArcId, Node<T, N>*, Node<T, N>*, LayerId);
		friend void Arc<T, N>::unlink();
		// Grants access to private Node constructor
		friend Node<T, N>* Graph<T, N>::buildNode(NodeId);
		friend Node<T, N>* Graph<T, N>::buildNode(NodeId id, T&& data);
		friend Node<T, N>* Graph<T, N>::buildNode(NodeId id, float weight, T&& data);

		// Allows graph to delete arcs from incoming / outgoing arcs lists
		friend void Graph<T, N>::removeNode(NodeId nodeId);

		protected:

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 */
		Node(NodeId id);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 */
		Node(NodeId id, T&& data);

		/**
		 * Node constructor.
		 *
		 * @param id node id
		 * @param weight node weight
		 * @param data node data
		 */
		Node(NodeId id, float weight, T&& data);

		private:
		T _data;
		float weight;
		std::array<Layer<T, N>, N> layers;
		Layer<T, N>& layer(LayerId layer);

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
		const Layer<T, N>& layer(LayerId layer) const;

		/**
		 * Returns a const reference to the complete array of layers associated to
		 * this node.
		 *
		 * @return const reference to this node's layers std::array
		 */
		const std::array<Layer<T, N>, N>& getLayers() const;

		/**
		 * Returns a vector containing pointers to this node's incoming arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to incoming arcs
		 */
		std::vector<Arc<T, N>*> getIncomingArcs() const;
		/**
		 * Returns the incoming neighbors list of this node, computed as source
		 * nodes of each incoming arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return incoming neighbors list
		 */
		std::vector<Node<T, N>*> inNeighbors() const;
		/**
		 * Returns a vector containing pointers to this node's outgoing arcs on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return pointers to outgoing arcs
		 */
		std::vector<Arc<T, N>*> getOutgoingArcs() const;
		/**
		 * Returns the outgoing neighbors list of this node, computed as target
		 * nodes of each outgoing arc on the
		 * default layer (layer associated to the value `0` in the `LayerType`
		 * enum).
		 *
		 * @return outgoing neighbors list
		 */
		std::vector<Node<T, N>*> outNeighbors() const;

	};

	template<typename T, int N> Node<T, N>::Node(NodeId id) : Node<T, N>(id, 1., T()) {
	}

	template<typename T, int N> Node<T, N>::Node(NodeId id, T&& data) : Node<T, N>(id, 1., std::forward<T>(data)) {
	}

	template<typename T, int N> Node<T, N>::Node(NodeId id, float weight, T&& data) : GraphItem(id), _data(std::forward<T>(data)), weight(weight) {
		for (int i = 0; i < N; i++) {
			this->layers[i] = Layer<T, N>();
		}
	}

	template<typename T, int N> T& Node<T, N>::data() {
		return this->_data;
	}

	template<typename T, int N> const T& Node<T, N>::data() const {
		return this->_data;
	}

	template<typename T, int N> float Node<T, N>::getWeight() const {
		return this->weight;
	}

	template<typename T, int N> void Node<T, N>::setWeight(float weight) {
		this->weight = weight;
	}

	template<typename T, int N> Layer<T, N>& Node<T, N>::layer(LayerId layer) {
		return this->layers[layer];
	}

	template<typename T, int N> const Layer<T, N>& Node<T, N>::layer(LayerId layer) const {
		return this->layers[layer];
	}

	template<typename T, int N> const std::array<Layer<T, N>, N>& Node<T, N>::getLayers() const {
		return this->layers;
	}

	template<typename T, int N> std::vector<Arc<T, N>*> Node<T, N>::getIncomingArcs() const {
		return this->layers[0].getIncomingArcs();
	}


	template<typename T, int N> std::vector<Node<T, N>*> Node<T, N>::inNeighbors() const {
		std::vector<Node<T, N>*> neighbors;
		for(auto arc : this->layers[0].getIncomingArcs())
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	template<typename T, int N> std::vector<Arc<T, N>*> Node<T, N>::getOutgoingArcs() const {
		return this->layers[0].getOutgoingArcs();
	}

	template<typename T, int N> std::vector<Node<T, N>*> Node<T, N>::outNeighbors() const {
		std::vector<Node<T, N>*> neighbors;
		for(auto arc : this->layers[0].getOutgoingArcs())
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}

#endif
