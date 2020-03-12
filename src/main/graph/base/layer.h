#ifndef LAYER_H
#define LAYER_H

#include <iostream>
#include "utils/macros.h"
#include <vector>
#include <array>

namespace FPMAS::graph::base {
	template<typename T, int N> class Arc;
	template<typename T, int N> class Node;
	template<typename T, int N> class Graph;

	/**
	 * Class representing a Layer of a Graph.
	 *
	 * Layers instances are used by each node to represent their _incoming_ and
	 * _outgoing_ arcs according to layers defined in the specified `LayerType`
	 * enum.
	 *
	 * @tparam T associated data type
	 * @tparam N number of layers
	 */
	template<typename T, int N> class Layer {
		friend Arc<T, N>::Arc(ArcId, Node<T, N>*, Node<T, N>*, LayerId);
		// Allows removeNode function to remove deleted arcs
		friend void Arc<T, N>::unlink();
		//friend void Graph<T, N>::unlink(Arc<T, N>*);

		protected:
			/**
			 * List of pointers to incoming arcs.
			 */
			std::vector<Arc<T, N>*> incomingArcs;
			/**
			 * List of pointers to outgoing arcs.
			 */
			std::vector<Arc<T, N>*> outgoingArcs;

		public:
			std::vector<Arc<T, N>*> getIncomingArcs() const;
			std::vector<Node<T, N>*> inNeighbors() const;
			std::vector<Arc<T, N>*> getOutgoingArcs() const;
			std::vector<Node<T, N>*> outNeighbors() const;
	};

	/**
	 * Returns a vector containing pointers to this node's incoming arcs.
	 *
	 * @return pointers to incoming arcs
	 */
	template<typename T, int N> std::vector<Arc<T, N>*> Layer<T, N>::getIncomingArcs() const {
		return this->incomingArcs;
	}

	/**
	 * Returns the incoming neighbors list of this node, computed as source
	 * nodes of each incoming arc.
	 *
	 * @return incoming neighbors list
	 */
	template<typename T, int N> std::vector<Node<T, N>*> Layer<T, N>::inNeighbors() const {
		std::vector<Node<T, N>*> neighbors;
		for(auto arc : this->incomingArcs)
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	/**
	 * Returns a vector containing pointers to this node's outgoing arcs.
	 *
	 * @return pointers to outgoing arcs
	 */
	template<typename T, int N> std::vector<Arc<T, N>*> Layer<T, N>::getOutgoingArcs() const {
		return this->outgoingArcs;
	}

	/**
	 * Returns the outgoing neighbors list of this node, computed as target
	 * nodes of each outgoing arc.
	 *
	 * @return outgoing neighbors list
	 */
	template<typename T, int N> std::vector<Node<T, N>*> Layer<T, N>::outNeighbors() const {
		std::vector<Node<T, N>*> neighbors;
		for(auto arc : this->outgoingArcs)
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}
#endif
