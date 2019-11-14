#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <nlohmann/json.hpp>
#include "serializers.h"

namespace FPMAS {
	namespace graph {
		/**
		 * A labelled object, super class for Nodes and Arcs.
		 */
		class GraphItem {
			private:
				std::string id;
			protected:
				GraphItem(std::string);
			public:
				std::string getId() const;
		};

		// Arc forward declaration to use in Node
		template<class T> class Node;
		template<class T> class Arc;

		/**
		 * Graph class.
		 */
		template<class T> class Graph;
		template<class T> void to_json(nlohmann::json&, const Graph<T>&);
		template<class T> class Graph {
			template<class S> friend void FPMAS::graph::to_json(nlohmann::json&, const Graph<S>&);

			private:
				std::unordered_map<std::string, Node<T>*> nodes;
				std::unordered_map<std::string, Arc<T>*> arcs;
			public:
				Node<T>* getNode(std::string) const;
				Arc<T>* getArc(std::string) const;
				Node<T>* buildNode(std::string id, T* data);
				Arc<T>* link(Node<T>* source, Node<T>* target, std::string arcLabel);
				~Graph();

		};

		/**
		 * Graph arc.
		 */
		template<class T> class Arc : public GraphItem {
			// Grants access to the Arc constructor
			friend Node<T>;
			friend Arc<T>* Graph<T>::link(Node<T>*, Node<T>*, std::string);

			private:
			Arc(std::string, Node<T>*, Node<T>*);
			Node<T>* sourceNode;
			Node<T>* targetNode;

			public:
			Node<T>* getSourceNode() const;
			Node<T>* getTargetNode() const;

		};

		/**
		 * Graph node.
		 */
		template<class T> class Node : public GraphItem {
			// Grants access to incoming and outgoing arcs lists
			friend Arc<T>::Arc(std::string, Node<T>*, Node<T>*);
			// Grants access to private Node constructor
			friend Node<T>* Graph<T>::buildNode(std::string id, T *data);

			private:
			Node(std::string, T*);
			T* data;
			std::vector<Arc<T>*> incomingArcs;
			std::vector<Arc<T>*> outgoingArcs;
			public:
			T* getData() const;
			std::vector<Arc<T>*> getIncomingArcs() const;
			std::vector<Arc<T>*> getOutgoingArcs() const;

		};

		/**
		 * Private arc constructor.
		 *
		 * @param id arc id
		 * @param sourceNode pointer to the source Node
		 * @param targetNode pointer to the target Node
		 */
		template<class T> Arc<T>::Arc(std::string id, Node<T> * sourceNode, Node<T> * targetNode)
			: GraphItem(id), sourceNode(sourceNode), targetNode(targetNode) {
				this->sourceNode->outgoingArcs.push_back(this);
				this->targetNode->incomingArcs.push_back(this);
			};

		/***********/
		/* Arc API */
		/***********/

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

		/************/
		/* Node API */
		/************/

		/**
		 * Node constructor.
		 *
		 * Responsability is left to the user to maintain the unicity of ids
		 * within each graph.
		 *
		 * @param std::string node id
		 * @param data pointer to node data
		 */
		template<class T> Node<T>::Node(std::string id, T* data) : GraphItem(id) {
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

		/*************/
		/* Graph API */
		/*************/

		/**
		 * Finds and returns the node associated to the specified id within this
		 * graph.
		 *
		 * @param id node id
		 * @return pointer to associated node
		 */
		template<class T> Node<T>* Graph<T>::getNode(std::string id) const {
			return this->nodes.find(id)->second;
		}

		/**
		 * Finds and returns the arc associated to the specified id within this
		 * graph.
		 *
		 * @param id arc id
		 * @return pointer to associated arc
		 */
		template<class T> Arc<T>* Graph<T>::getArc(std::string id) const {
			return this->arcs.find(id)->second;
		}

		/**
		 * Builds a node with the specify id and data, and adds it to this graph,
		 * and finally returns the built node.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 * @return pointer to build node
		 */
		template<class T> Node<T>* Graph<T>::buildNode(std::string id, T *data) {
			Node<T>* node = new Node<T>(id, data);
			this->nodes[id] = node;
			return node;
		}

		/**
		 * Builds a directed arc with the specified id from the source node to the
		 * target node, adds it to the graph and finally returns the built arc.
		 *
		 * @param source pointer to source node
		 * @param target pointer to target node
		 * @param arcLabel arc id
		 * @return built arc
		 */
		template<class T> Arc<T>* Graph<T>::link(Node<T> *source, Node<T> *target, std::string arcLabel) {
			Arc<T>* arc = new Arc<T>(arcLabel, source, target);
			this->arcs[arcLabel] = arc;
			return arc;
		}

		/**
		 * Graph destructor.
		 *
		 * Deletes nodes and arcs built in this graph.
		 */
		template<class T> Graph<T>::~Graph() {
			for(auto node : this->nodes) {
				delete node.second;
			}
			for(auto arc : this->arcs) {
				delete arc.second;
			}
		}
	}
}

#endif
