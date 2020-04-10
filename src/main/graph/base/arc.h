#ifndef ARC_H
#define ARC_H

#include <nlohmann/json.hpp>

#include <array>
#include "graph_item.h"
#include "graph.h"
#include "node.h"
#include "layer.h"

namespace FPMAS::graph::base {

	template<typename, typename> class Graph;

	/**
	 * Graph arc.
	 *
	 * @tparam T associated data type
	 * @tparam N number of layers
	 */
	template<typename T, typename IdType>
	class Arc : public GraphItem<IdType> {
		// Grants access to the Arc constructor
		friend Node<T, IdType>;
		friend Layer<T, IdType>;
		friend Arc<T, IdType>* Graph<T, IdType>::_link(IdType, Node<T, IdType>*, Node<T, IdType>*, LayerId);
		friend Arc<T, IdType> nlohmann::adl_serializer<Arc<T, IdType>>::from_json(const json&);
		friend void Graph<T, IdType>::unlink(Arc<T, IdType> *);

		protected:
		Arc(IdType, Node<T, IdType>*, Node<T, IdType>*);
		Arc(IdType, Node<T, IdType>*, Node<T, IdType>*, LayerId layer);
		/**
		 * Pointer to source Node.
		 */
		Node<T, IdType>* sourceNode;
		/**
		 * Pointer to target Node.
		 */
		Node<T, IdType>* targetNode;

		public:
		/**
		 * Layer of this arc.
		 */
		const LayerId layer;

		float getWeight() const {return 1.;};

		Node<T, IdType>* const getSourceNode() const;
		Node<T, IdType>* const getTargetNode() const;

		void unlink();
	};

	/**
	 * Arc constructor.
	 *
	 * @param id arc id
	 * @param sourceNode pointer to the source Node
	 * @param targetNode pointer to the target Node
	 * @param layer arc layer
	 */
	template<typename T, typename IdType> Arc<T, IdType>::Arc(IdType id, Node<T, IdType>* sourceNode, Node<T, IdType>* targetNode, LayerId layer)
		: GraphItem<IdType>(id), sourceNode(sourceNode), targetNode(targetNode), layer(layer) {
			this->sourceNode->_layer(layer).outgoingArcs.push_back(this);
			this->targetNode->_layer(layer).incomingArcs.push_back(this);
		};

	/**
	 * Arc constructor for the default layer (layer associated to the value `0`
	 * in the `LayerType` enum).
	 *
	 * @param id arc id
	 * @param sourceNode pointer to the source Node
	 * @param targetNode pointer to the target Node
	 */
	template<typename T, typename IdType> Arc<T, IdType>::Arc(IdType id, Node<T, IdType>* sourceNode, Node<T, IdType>* targetNode) : Arc(id, sourceNode, targetNode, DefaultLayer) {

	}

	/**
	 * Unlinks this arc from its source and target nodes.
	 *
	 * The arc is removed from the incoming / outgoing arcs list of its target
	 * / source nodes respectively. However, the source and target nodes
	 * pointer of this arcs stay valid. In consequence, this function is
	 * usefull to unlink / delete arcs at the Graph scale.
	 */
	template<typename T, typename IdType> void Arc<T, IdType>::unlink() {
		auto& sourceArcs = sourceNode->_layer(this->layer).outgoingArcs;
		for(auto it = sourceArcs.begin(); it != sourceArcs.end();) {
			if(*it == this) {
				sourceArcs.erase(it);
				it = sourceArcs.end();
			} else {
				it++;
			}
		}
		auto& targetArcs = targetNode->_layer(this->layer).incomingArcs;
		for(auto it = targetArcs.begin(); it != targetArcs.end();) {
			if(*it == this) {
				targetArcs.erase(it);
				it = targetArcs.end();
			} else {
				it++;
			}
		}
	}
	/**
	 * Returns a pointer to the source node of this arc.
	 *
	 * @return pointer to the source node
	 */
	template<typename T, typename IdType> Node<T, IdType>* const Arc<T, IdType>::getSourceNode() const {
		return this->sourceNode;
	}

	/**
	 * Returns a pointer to the target node of this arc.
	 *
	 * @return pointer to the target node
	 */
	template<typename T, typename IdType> Node<T, IdType>* const Arc<T, IdType>::getTargetNode() const {
		return this->targetNode;
	}
}

#endif
