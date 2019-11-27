#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"

namespace FPMAS {
	namespace graph {

		template<class T> class Graph;
		template<class T> class Arc;

		/**
		 * Graph node.
		 *
		 * @tparam T associated data type
		 */
		template<class T> class Node : public GraphItem {
			friend Node<T> nlohmann::adl_serializer<Node<T>>::from_json(const json&);
			friend Arc<T> nlohmann::adl_serializer<Arc<T>>::from_json(const json&);
			// Grants access to incoming and outgoing arcs lists
			friend Arc<T>::Arc(unsigned long, Node<T>*, Node<T>*);
			// Grants access to private Node constructor
			friend Node<T>* Graph<T>::buildNode(unsigned long);
			friend Node<T>* Graph<T>::buildNode(unsigned long id, T *data);
			friend Node<T>* Graph<T>::buildNode(unsigned long id, float weight, T *data);
			// Allows removeNode function to remove deleted arcs
			friend void Graph<T>::unlink(Arc<T>*);

			private:
			Node(unsigned long);
			Node(unsigned long, T*);
			Node(unsigned long, float, T*);
			T* data;
			float weight = 1.;

			protected:
			/**
			 * List of pointers to incoming arcs.
			 */
			std::vector<Arc<T>*> incomingArcs;
			/**
			 * List of pointers to outgoing arcs.
			 */
			std::vector<Arc<T>*> outgoingArcs;

			public:
			T* getData() const;
			void setData(T*);
			float getWeight() const;
			void setWeight(float);
			std::vector<Arc<T>*> getIncomingArcs() const;
			std::vector<Arc<T>*> getOutgoingArcs() const;

		};


		/************/
		/* Node API */
		/************/

		template<class T> Node<T>::Node(unsigned long id) : GraphItem(id) {
		}

		/**
		 * Node constructor.
		 *
		 * Responsability is left to the user to maintain the unicity of ids
		 * within each graph.
		 *
		 * @param unsigned long node id
		 * @param data pointer to node data
		 */
		template<class T> Node<T>::Node(unsigned long id, T* data) : GraphItem(id) {
			this->data = data;
		}

		template<class T> Node<T>::Node(unsigned long id, float weight, T* data) : GraphItem(id) {
			this->weight = weight;
			this->data = data;
		}

		/**
		 * Returns node's data.
		 *
		 * @return pointer to node's data
		 */
		template<class T> T* Node<T>::getData() const {
			return this->data;
		}

		/**
		 * Sets node's data to the specified value.
		 *
		 * @param data pointer to node data
		 */
		template<class T> void Node<T>::setData(T* data) {
			this->data = data;
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
		template<class T> float Node<T>::getWeight() const {
			return this->weight;
		}

		/**
		 * Sets node's weight to the specified value.
		 *
		 * @param weight
		 */
		template<class T> void Node<T>::setWeight(float weight) {
			this->weight = weight;
		}

		/**
		 * Returns a vector containing pointers to this node's incoming arcs.
		 *
		 * @return pointers to incoming arcs
		 */
		template<class T> std::vector<Arc<T>*> Node<T>::getIncomingArcs() const {
			return this->incomingArcs;
		}

		/**
		 * Returns a vector containing pointers to this node's outgoing arcs.
		 *
		 * @return pointers to outgoing arcs
		 */
		template<class T> std::vector<Arc<T>*> Node<T>::getOutgoingArcs() const {
			return this->outgoingArcs;
		}
	}
}

#endif
