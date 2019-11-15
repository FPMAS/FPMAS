#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "serializers.h"

namespace FPMAS {
	/**
	 * The FPMAS::graph namespace.
	 */
	namespace graph {
		/**
		 * A labelled object, super class for Nodes and Arcs.
		 */
		class GraphItem {
			private:
				unsigned long id;
			protected:
				GraphItem(unsigned long);
			public:
				unsigned long getId() const;
		};

		// Forward declaration
		template<class T> class Node;
		template<class T> class Arc;
		template<class T> class Graph;

		/**
		 * Graph class.
		 *
		 * Aditionnaly to the functions defined in the Graph API, it is
		 * interesting to notice that Graphs object can be json serialized and
		 * de-serialized using the [JSON for Modern C++
		 * library](https://github.com/nlohmann/json).
		 *
		 * For example, the following code :
		 *
		 * ```
		 *  #include <iostream>
		 *  #include "environment/graph.h"
		 *  #include <nlohmann/json.hpp>
		 *
		 *  using FPMAS::graph::Graph;
		 *  using FPMAS::graph::Node;
		 *
		 *  int main() {
		 *  	Graph<int> g;
		 *		Node<int>* n1 = g.buildNode("0", new int(0));
		 *		Node<int>* n2 = g.buildNode("1", new int(1));
		 *		g.link(n1, n2, "0");
		 *
		 *		json j = g;
		 *
		 *		std::out << j.dump() << std::endl;
		 *
		 *		delete n1->getData();
		 *		delete n2->getData();
		 *	}
		 *	```
		 *  Will print the following json :
		 *	```
		 *	{"arcs":[{"id":"0","link":["0","1"]}],"nodes":[{"data":0,"id":"0"},{"data":1,"id":"1"}]}
		 *	```
		 *  For more details about how json format used, see the
		 *  FPMAS::graph::to_json(json&, const Graph<T>&) documentation.
		 *
		 *  Graph can also be un-serialized from json files with the same
		 *  format.
		 *
		 *  The following example code will build the graph serialized in the
		 *  previous example :
		 *	```
		 *	#include <iostream>
		 *	#include "environment/graph.h"
		 *	#include <nlohmann/json.hpp>
		 *
		 *	using FPMAS::graph::Graph;
		 *	using FPMAS::graph::Node;
		 *
		 *	int main() {
		 *		json graph_json = R"(
		 *			{
		 *			"arcs":[
		 *				{"id":"0","link":["0","1"]}
		 *				],
		 *			"nodes":[
		 *				{"data":0,"id":"0"},
		 *				{"data":1,"id":"1"}
		 *				]
		 *			}
		 *			)"_json;
		 *
		 * 		// Unserialize the graph from the json string
		 *		Graph<int> graph = graph_json.get<Graph<int>>();
		 *	}
		 *	```
		 *
		 * For more information about the json unserialization, see the [JSON
		 * for Modern C++ documentation]
		 * (https://github.com/nlohmann/json/blob/develop/README.md)
		 * and the FPMAS::graph::from_json(const json&, Graph<T>&)
		 * function.
		 *
		 * @tparam T associated data type
		 */
		template<class T> class Graph {
			protected:
				std::unordered_map<unsigned long, Node<T>*> nodes;
				std::unordered_map<unsigned long, Arc<T>*> arcs;
			public:
				Node<T>* getNode(unsigned long) const;
				std::unordered_map<unsigned long, Node<T>*> getNodes() const;
				Arc<T>* getArc(unsigned long) const;
				std::unordered_map<unsigned long, Arc<T>*> getArcs() const;
				Node<T>* buildNode(unsigned long id, T* data);
				Node<T>* buildNode(unsigned long id, float weight, T* data);
				Arc<T>* link(Node<T>* source, Node<T>* target, unsigned long arcLabel);
				~Graph();

		};

		/**
		 * Graph arc.
		 *
		 * @tparam T associated data type
		 */
		template<class T> class Arc : public GraphItem {
			// Grants access to the Arc constructor
			friend Node<T>;
			friend Arc<T>* Graph<T>::link(Node<T>*, Node<T>*, unsigned long);

			private:
			Arc(unsigned long, Node<T>*, Node<T>*);
			Node<T>* sourceNode;
			Node<T>* targetNode;

			public:
			Node<T>* getSourceNode() const;
			Node<T>* getTargetNode() const;

		};

		/**
		 * Graph node.
		 *
		 * @tparam T associated data type
		 */
		template<class T> class Node : public GraphItem {
			// Grants access to incoming and outgoing arcs lists
			friend Arc<T>::Arc(unsigned long, Node<T>*, Node<T>*);
			// Grants access to private Node constructor
			friend Node<T>* Graph<T>::buildNode(unsigned long id, T *data);
			friend Node<T>* Graph<T>::buildNode(unsigned long id, float weight, T *data);

			private:
			Node(unsigned long, T*);
			Node(unsigned long, float, T*);
			T* data;
			float weight = 1.;
			std::vector<Arc<T>*> incomingArcs;
			std::vector<Arc<T>*> outgoingArcs;

			public:
			T* getData() const;
			float getWeight() const;
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
		template<class T> Arc<T>::Arc(unsigned long id, Node<T> * sourceNode, Node<T> * targetNode)
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

		template<class T> float Node<T>::getWeight() const {
			return this->weight;
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
		template<class T> Node<T>* Graph<T>::getNode(unsigned long id) const {
			return this->nodes.find(id)->second;
		}

		/**
		 * Returns a copy of the nodes map of this graph.
		 *
		 * @return nodes contained in this graph
		 */
		template<class T> std::unordered_map<unsigned long, Node<T>*> Graph<T>::getNodes() const {
			return this->nodes;
		}

		/**
		 * Finds and returns the arc associated to the specified id within this
		 * graph.
		 *
		 * @param id arc id
		 * @return pointer to associated arc
		 */
		template<class T> Arc<T>* Graph<T>::getArc(unsigned long id) const {
			return this->arcs.find(id)->second;
		}

		/**
		 * Returns a copy of the arcs map of this graph.
		 *
		 * @return arcs contained in this graph
		 */
		template<class T> std::unordered_map<unsigned long, Arc<T>*> Graph<T>::getArcs() const {
			return this->arcs;
		}

		/**
		 * Builds a node with the specify id and data, and adds it to this graph,
		 * and finally returns the built node.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 * @return pointer to build node
		 */
		template<class T> Node<T>* Graph<T>::buildNode(unsigned long id, T *data) {
			Node<T>* node = new Node<T>(id, data);
			this->nodes[id] = node;
			return node;
		}

		/**
		 * Builds a node with the specify id and data, and adds it to this graph,
		 * and finally returns the built node.
		 *
		 * @param id node id
		 * @param data pointer to node data
		 * @return pointer to build node
		 */
		template<class T> Node<T>* Graph<T>::buildNode(unsigned long id, float weight, T *data) {
			Node<T>* node = new Node<T>(id, weight, data);
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
		template<class T> Arc<T>* Graph<T>::link(Node<T> *source, Node<T> *target, unsigned long arcLabel) {
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
