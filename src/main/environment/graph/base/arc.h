#ifndef ARC_H
#define ARC_H

#include <nlohmann/json.hpp>

#include "graph_item.h"
#include "graph.h"
#include "node.h"

namespace FPMAS {
	namespace graph {

		template<class T> class Graph;
		template<class T> class Node;

		/**
		 * Graph arc.
		 *
		 * @tparam T associated data type
		 */
		template<class T> class Arc : public GraphItem {
			// Grants access to the Arc constructor
			friend Node<T>;
			friend Arc<T>* Graph<T>::link(Node<T>*, Node<T>*, unsigned long);
			friend Arc<T> nlohmann::adl_serializer<Arc<T>>::from_json(const json&);

			protected:
			Arc(unsigned long, Node<T>*, Node<T>*);
			/**
			 * Pointer to source Node.
			 */
			Node<T>* sourceNode;
			/**
			 * Pointer to target Node.
			 */
			Node<T>* targetNode;

			public:
			Node<T>* getSourceNode() const;
			Node<T>* getTargetNode() const;

		};

		/**
		 * Private arc constructor.
		 *
		 * @param id arc id
		 * @param sourceNode pointer to the source Node
		 * @param targetNode pointer to the target Node
		 */
		template<class T> Arc<T>::Arc(unsigned long id, Node<T> * sourceNode, Node<T> * targetNode)
			: GraphItem(id), sourceNode(sourceNode), targetNode(targetNode) {
				this->sourceNode->outgoingArcs.push_back(this);
				this->targetNode->incomingArcs.push_back(this);
			};

		/**
		 * Returns the source node of this arc.
		 *
		 * @return pointer to the source node
		 */
		template<class T> Node<T>* Arc<T>::getSourceNode() const {
			return this->sourceNode;
		}

		/**
		 * Returns the target node of this arc.
		 *
		 * @return pointer to the target node
		 */
		template<class T> Node<T>* Arc<T>::getTargetNode() const {
			return this->targetNode;
		}
	}
}

#endif
