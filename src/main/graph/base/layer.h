#ifndef LAYER_H
#define LAYER_H

#include <iostream>
#include "utils/macros.h"
#include <vector>
#include <array>

namespace FPMAS::graph::base {
	template<typename, typename> class Arc;
	template<typename, typename> class Node;
	template<typename, typename> class Graph;

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
	template<typename T, typename IdType> class Layer {
		friend Arc<T, IdType>::Arc(IdType, Node<T, IdType>*, Node<T, IdType>*, LayerId);
		// Allows removeNode function to remove deleted arcs
		friend void Arc<T, IdType>::unlink();
		//friend void Graph<T, N>::unlink(Arc<T, IdType>*);

		protected:
			/**
			 * List of pointers to incoming arcs.
			 */
			std::vector<Arc<T, IdType>*> incomingArcs;
			/**
			 * List of pointers to outgoing arcs.
			 */
			std::vector<Arc<T, IdType>*> outgoingArcs;

		public:
			std::vector<Arc<T, IdType>*> getIncomingArcs() const;
			std::vector<Node<T, IdType>*> inNeighbors() const;
			std::vector<Arc<T, IdType>*> getOutgoingArcs() const;
			std::vector<Node<T, IdType>*> outNeighbors() const;
	};

	/**
	 * Returns a vector containing pointers to this node's incoming arcs.
	 *
	 * @return pointers to incoming arcs
	 */
	template<typename T, typename IdType> std::vector<Arc<T, IdType>*> Layer<T, IdType>::getIncomingArcs() const {
		return this->incomingArcs;
	}

	/**
	 * Returns the incoming neighbors list of this node, computed as source
	 * nodes of each incoming arc.
	 *
	 * @return incoming neighbors list
	 */
	template<typename T, typename IdType> std::vector<Node<T, IdType>*> Layer<T, IdType>::inNeighbors() const {
		std::vector<Node<T, IdType>*> neighbors;
		for(auto arc : this->incomingArcs)
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	/**
	 * Returns a vector containing pointers to this node's outgoing arcs.
	 *
	 * @return pointers to outgoing arcs
	 */
	template<typename T, typename IdType> std::vector<Arc<T, IdType>*> Layer<T, IdType>::getOutgoingArcs() const {
		return this->outgoingArcs;
	}

	/**
	 * Returns the outgoing neighbors list of this node, computed as target
	 * nodes of each outgoing arc.
	 *
	 * @return outgoing neighbors list
	 */
	template<typename T, typename IdType> std::vector<Node<T, IdType>*> Layer<T, IdType>::outNeighbors() const {
		std::vector<Node<T, IdType>*> neighbors;
		for(auto arc : this->outgoingArcs)
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}
#endif
