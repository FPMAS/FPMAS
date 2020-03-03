#ifndef ARC_H
#define ARC_H

#include <nlohmann/json.hpp>

#include <array>
#include "graph_item.h"
#include "graph.h"
#include "node.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<typename T, int N> class Graph;

	/**
	 * Graph arc.
	 *
	 * @tparam T associated data type
	 * @tparam N number of layers
	 */
	template<typename T, int N>
	class Arc : public GraphItem {
		// Grants access to the Arc constructor
		friend Node<T, N>;
		friend Layer<T, N>;
		friend Arc<T, N>* Graph<T, N>::link(Node<T, N>*, Node<T, N>*, unsigned long, LayerId);
		friend Arc<T, N> nlohmann::adl_serializer<Arc<T, N>>::from_json(const json&);

		protected:
		Arc(unsigned long, Node<T, N>*, Node<T, N>*);
		Arc(unsigned long, Node<T, N>*, Node<T, N>*, LayerId layer);
		/**
		 * Pointer to source Node.
		 */
		Node<T, N>* sourceNode;
		/**
		 * Pointer to target Node.
		 */
		Node<T, N>* targetNode;

		public:
		/**
		 * Layer of this arc.
		 */
		const LayerId layer;

		Node<T, N>* getSourceNode() const;
		Node<T, N>* getTargetNode() const;

	};

	/**
	 * Arc constructor.
	 *
	 * @param id arc id
	 * @param sourceNode pointer to the source Node
	 * @param targetNode pointer to the target Node
	 * @param layer arc layer
	 */
	template<typename T, int N> Arc<T, N>::Arc(unsigned long id, Node<T, N>* sourceNode, Node<T, N>* targetNode, LayerId layer)
		: GraphItem(id), sourceNode(sourceNode), targetNode(targetNode), layer(layer) {
			this->sourceNode->layer(layer).outgoingArcs.push_back(this);
			this->targetNode->layer(layer).incomingArcs.push_back(this);
		};

	/**
	 * Arc constructor for the default layer (layer associated to the value `0`
	 * in the `LayerType` enum).
	 *
	 * @param id arc id
	 * @param sourceNode pointer to the source Node
	 * @param targetNode pointer to the target Node
	 */
	template<typename T, int N> Arc<T, N>::Arc(unsigned long id, Node<T, N>* sourceNode, Node<T, N>* targetNode) : Arc(id, sourceNode, targetNode, DefaultLayer) {

	}
	/**
	 * Returns a pointer to the source node of this arc.
	 *
	 * @return pointer to the source node
	 */
	template<typename T, int N> Node<T, N>* Arc<T, N>::getSourceNode() const {
		return this->sourceNode;
	}

	/**
	 * Returns a pointer to the target node of this arc.
	 *
	 * @return pointer to the target node
	 */
	template<typename T, int N> Node<T, N>* Arc<T, N>::getTargetNode() const {
		return this->targetNode;
	}
}

#endif
