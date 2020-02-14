#ifndef LAYER_H
#define LAYER_H

#include "utils/macros.h"
#include <vector>
#include <array>

enum DefaultLayer {
	Default = 0
};
static const std::array<DefaultLayer, 1> DefaultLayers = { Default };


namespace FPMAS::graph::base {
	template<NODE_PARAMS> class Arc;
	template<NODE_PARAMS> class Node;
	template<NODE_PARAMS> class Graph;

	template<NODE_PARAMS> class Layer {
		friend ARC::Arc(unsigned long, NODE*, NODE*, LayerType);
		// Allows removeNode function to remove deleted arcs
		friend bool Graph<NODE_PARAMS_SPEC>::unlink(ARC*);

		protected:
			/**
			 * List of pointers to incoming arcs.
			 */
			std::vector<ARC*> incomingArcs;
			/**
			 * List of pointers to outgoing arcs.
			 */
			std::vector<ARC*> outgoingArcs;

		public:
			std::vector<ARC*> getIncomingArcs() const;
			std::vector<NODE*> inNeighbors() const;
			std::vector<ARC*> getOutgoingArcs() const;
			std::vector<NODE*> outNeighbors() const;

	};

	/**
	 * Returns a vector containing pointers to this node's incoming arcs.
	 *
	 * @return pointers to incoming arcs
	 */
	template<NODE_PARAMS> std::vector<ARC*> Layer<NODE_PARAMS_SPEC>::getIncomingArcs() const {
		return this->incomingArcs;
	}

	/**
	 * Returns the incoming neighbors list of this node, computed as source
	 * nodes of each incoming arc.
	 *
	 * @return incoming neighbors list
	 */
	template<NODE_PARAMS> std::vector<NODE*> Layer<NODE_PARAMS_SPEC>::inNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->incomingArcs)
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	/**
	 * Returns a vector containing pointers to this node's outgoing arcs.
	 *
	 * @return pointers to outgoing arcs
	 */
	template<NODE_PARAMS> std::vector<ARC*> Layer<NODE_PARAMS_SPEC>::getOutgoingArcs() const {
		return this->outgoingArcs;
	}

	/**
	 * Returns the outgoing neighbors list of this node, computed as target
	 * nodes of each outgoing arc.
	 *
	 * @return outgoing neighbors list
	 */
	template<NODE_PARAMS> std::vector<NODE*> Layer<NODE_PARAMS_SPEC>::outNeighbors() const {
		std::vector<NODE*> neighbors;
		for(auto arc : this->outgoingArcs)
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}

}
#endif
