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

	/**
	 * Graph node.
	 *
	 * @tparam T associated data type
	 */
	template<
		class T,
		typename LayerType = DefaultLayer,
		int N = 1
	>
	class Node : public GraphItem {
		friend NODE nlohmann::adl_serializer<NODE>::from_json(const json&);
		friend ARC nlohmann::adl_serializer<ARC>::from_json(const json&);
		// Grants access to incoming and outgoing arcs lists
		friend ARC::Arc(unsigned long, NODE*, NODE*, LayerType);
		// Grants access to private Node constructor
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long);
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, T data);
		friend NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, float weight, T data);
		// Grants access to layers
		friend FossilArcs<ARC> Graph<NODE_PARAMS_SPEC>::removeNode(unsigned long);
		// Allows removeNode function to remove deleted arcs
		friend bool Graph<NODE_PARAMS_SPEC>::unlink(ARC*);

		protected:
		Node(unsigned long);
		Node(unsigned long, T);
		Node(unsigned long, float, T);

		private:
		T _data;
		float weight;
		std::array<Layer<NODE_PARAMS_SPEC>, N> layers;
		Layer<NODE_PARAMS_SPEC>& layer(LayerType layer);

		// protected:
		/**
		 * List of pointers to incoming arcs.
		 */
		// std::vector<ARC*> incomingArcs;
		/**
		 * List of pointers to outgoing arcs.
		 */
		// std::vector<ARC*> outgoingArcs;

		public:
		/**
		 * Reference to node's data.
		 *
		 * @return reference to node's data
		 */
		T& data();
		/**
		 * Const feference to node's data.
		 *
		 * @return const reference to node's data
		 */
		const T& data() const;

		float getWeight() const;
		void setWeight(float);

		const Layer<NODE_PARAMS_SPEC>& layer(LayerType layer) const;

		std::vector<ARC*> getIncomingArcs() const;
		std::vector<NODE*> inNeighbors() const;
		std::vector<ARC*> getOutgoingArcs() const;
		std::vector<NODE*> outNeighbors() const;

	};

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 */
	template<NODE_PARAMS> NODE::Node(unsigned long id) : NODE(id, 1., T()) {
	}

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 * @param data pointer to node data
	 */
	template<NODE_PARAMS> NODE::Node(unsigned long id, T data) : NODE(id, 1., data) {
	}

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 * @param weight node weight
	 * @param data node data
	 */
	template<NODE_PARAMS> NODE::Node(unsigned long id, float weight, T data) : GraphItem(id), _data(data), weight(weight) {
		for (int i = 0; i < N; i++) {
			this->layers[i] = Layer<NODE_PARAMS_SPEC>();
		}
	}


	template<NODE_PARAMS> T& NODE::data() {
		return this->_data;
	}

	template<NODE_PARAMS> const T& NODE::data() const {
		return this->_data;
	}

	/**
	 * Returns node's weight
	 *
	 * Default value set to 1., if weight has not been provided by the
	 * user. This weight might for example be used by graph partitionning
	 * functions to distribute the graph.
	 *
	 * @return node's weight
	 */
	template<NODE_PARAMS> float NODE::getWeight() const {
		return this->weight;
	}

	/**
	 * Sets node's weight to the specified value.
	 *
	 * @param weight
	 */
	template<NODE_PARAMS> void NODE::setWeight(float weight) {
		this->weight = weight;
	}

	template<NODE_PARAMS> Layer<NODE_PARAMS_SPEC>& NODE::layer(LayerType layer) {
		return this->layers[layer];
	}

	template<NODE_PARAMS> const Layer<NODE_PARAMS_SPEC>& NODE::layer(LayerType layer) const {
		return this->layers[layer];
	}

	/**
	 * Returns a vector containing pointers to this node's incoming arcs.
	 *
	 * @return pointers to incoming arcs
	 */
	template<NODE_PARAMS> std::vector<ARC*> NODE::getIncomingArcs() const {
		return this->layers[0].getIncomingArcs();
	}

	/**
	 * Returns the incoming neighbors list of this node, computed as source
	 * nodes of each incoming arc.
	 *
	 * @return incoming neighbors list
	 */
	template<NODE_PARAMS> std::vector<NODE*> NODE::inNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->layers[0].getIncomingArcs())
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	/**
	 * Returns a vector containing pointers to this node's outgoing arcs.
	 *
	 * @return pointers to outgoing arcs
	 */
	template<NODE_PARAMS> std::vector<ARC*> NODE::getOutgoingArcs() const {
		return this->layers[0].getOutgoingArcs();
	}

	/**
	 * Returns the outgoing neighbors list of this node, computed as target
	 * nodes of each outgoing arc.
	 *
	 * @return outgoing neighbors list
	 */
	template<NODE_PARAMS> std::vector<NODE*> NODE::outNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->layers[0].getOutgoingArcs())
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}

#endif
