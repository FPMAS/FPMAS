#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>

#include "graph_item.h"
#include "graph.h"
#include "arc.h"
#include "fossil.h"

namespace FPMAS::graph::base {

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
		friend Node<T>* Graph<T>::buildNode(unsigned long id, T data);
		friend Node<T>* Graph<T>::buildNode(unsigned long id, float weight, T data);
		// Allows removeNode function to remove deleted arcs
		friend bool Graph<T>::unlink(Arc<T>*);

		protected:
		Node(unsigned long);
		Node(unsigned long, T);
		Node(unsigned long, float, T);

		private:
		T _data;
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
		std::vector<Arc<T>*> getIncomingArcs() const;
		std::vector<Node<T>*> inNeighbors() const;
		std::vector<Arc<T>*> getOutgoingArcs() const;
		std::vector<Node<T>*> outNeighbors() const;

	};

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 */
	template<class T> Node<T>::Node(unsigned long id) : GraphItem(id) {
	}

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 * @param data pointer to node data
	 */
	template<class T> Node<T>::Node(unsigned long id, T data) : GraphItem(id), _data(data) {
	}

	/**
	 * Node constructor.
	 *
	 * @param id node id
	 * @param weight node weight
	 * @param data node data
	 */
	template<class T> Node<T>::Node(unsigned long id, float weight, T data) : GraphItem(id), weight(weight), _data(data) {
	}


	template<class T> T& Node<T>::data() {
		return this->_data;
	}

	template<class T> const T& Node<T>::data() const {
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

	template<class T> std::vector<Node<T>*> Node<T>::inNeighbors() const {
		std::vector<Node<T>*> neighbors;
		for(auto arc : this->incomingArcs)
			neighbors.push_back(arc->getSourceNode());
		return neighbors;
	}

	/**
	 * Returns a vector containing pointers to this node's outgoing arcs.
	 *
	 * @return pointers to outgoing arcs
	 */
	template<class T> std::vector<Arc<T>*> Node<T>::getOutgoingArcs() const {
		return this->outgoingArcs;
	}

	template<class T> std::vector<Node<T>*> Node<T>::outNeighbors() const {
		std::vector<Node<T>*> neighbors;
		for(auto arc : this->outgoingArcs)
			neighbors.push_back(arc->getTargetNode());
		return neighbors;
	}
}

#endif
