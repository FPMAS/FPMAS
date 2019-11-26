#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "serializers.h"

#include <nlohmann/json.hpp>

namespace FPMAS {
	/**
	 * The FPMAS::graph namespace contains Graph, Node and Arc definitions.
	 */
	namespace graph {
		/**
		 * A labelled object, super class for Nodes and Arcs.
		 */
		class GraphItem {
			private:
				unsigned long id;
			protected:
				GraphItem() {};
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
			friend Graph<T> nlohmann::adl_serializer<Graph<T>>::from_json(const json&);
			private:
				std::unordered_map<unsigned long, Node<T>*> nodes;
				std::unordered_map<unsigned long, Arc<T>*> arcs;
				void removeArc(Arc<T>*, std::vector<Arc<T>*>*);

			protected:
				Node<T>* buildNode(Node<T> node);

			public:
				Node<T>* getNode(unsigned long) const;
				std::unordered_map<unsigned long, Node<T>*> getNodes() const;
				Arc<T>* getArc(unsigned long) const;
				std::unordered_map<unsigned long, Arc<T>*> getArcs() const;
				Node<T>* buildNode(unsigned long id, T* data);
				Node<T>* buildNode(unsigned long id, float weight, T* data);
				Arc<T>* link(Node<T>* source, Node<T>* target, unsigned long arcLabel);
				Arc<T>* link(unsigned long source_id, unsigned long target_id, unsigned long arcLabel);
				Arc<T>* link(Arc<T>);
				void unlink(unsigned long);
				void unlink(Arc<T>*);
				void removeNode(unsigned long);
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
			friend Arc<T> nlohmann::adl_serializer<Arc<T>>::from_json(const json&);

			protected:
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
			friend Node<T> nlohmann::adl_serializer<Node<T>>::from_json(const json&);
			friend Arc<T> nlohmann::adl_serializer<Arc<T>>::from_json(const json&);
			// Grants access to incoming and outgoing arcs lists
			friend Arc<T>::Arc(unsigned long, Node<T>*, Node<T>*);
			// Grants access to private Node constructor
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
			std::vector<Arc<T>*> incomingArcs;
			std::vector<Arc<T>*> outgoingArcs;

			public:
			T* getData() const;
			void setData(T*);
			float getWeight() const;
			void setWeight(float);
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
		 * Builds a node by copy from the specified node. Memory is dynamically
		 * allocated for the node copy, and normally deleted when the graph is
		 * destroyed.
		 *
		 * @param node node to add to the graph
		 * @return built node (copy of the input node)
		 */
		template<class T> Node<T>* Graph<T>::buildNode(Node<T> node) {
			Node<T>* node_copy = new Node<T>(node);
			this->nodes[node_copy->getId()] = node_copy;
			return node_copy;
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
		 * Builds a node with the specify id weight and data, and adds it to this graph,
		 * and finally returns the built node.
		 *
		 * @param id node's id
		 * @param weight node's weight
		 * @param data pointer to node's data
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
		 * @param arc_id arc id
		 * @return built arc
		 */
		template<class T> Arc<T>* Graph<T>::link(Node<T> *source, Node<T> *target, unsigned long arc_id) {
			Arc<T>* arc = new Arc<T>(arc_id, source, target);
			this->arcs[arc_id] = arc;
			return arc;
		}

		/**
		 * Same as link(Node<T>*, Node<T>*, unsigned long), but uses node ids
		 * instead to retrieve source and target nodes in the current graph.
		 *
		 * @param source_id source node id
		 * @param target_id target node id
		 * @param arc_id new arc id
		 * @return built arc
		 */
		template<class T> Arc<T>* Graph<T>::link(unsigned long source_id, unsigned long target_id, unsigned long arc_id) {
			return this->link(this->getNode(source_id), this->getNode(target_id), arc_id);
		}

		template<class T> Arc<T>* Graph<T>::link(Arc<T> tempArc) {
			Arc<T>* arc = this->link(
				this->getNode(tempArc.getSourceNode()->getId()),
				this->getNode(tempArc.getTargetNode()->getId()),
				tempArc.getId()
				);
			delete tempArc.getSourceNode();
			delete tempArc.getTargetNode();
			return arc;
		}

		template<class T> void Graph<T>::removeArc(Arc<T>* arc, std::vector<Arc<T>*>* arcList) {
			for(auto it = arcList->begin(); it != arcList->end();) {
				if(*it == arc) {
					arcList->erase(it);
					it = arcList->end();
				}
				else {
					it++;
				}
			}
		}

		template<class T> void Graph<T>::unlink(unsigned long arcId) {
			this->unlink(this->arcs.at(arcId));
		}

		template<class T> void Graph<T>::unlink(Arc<T>* arc) {
				Node<T>* source_node = arc->getSourceNode();
				std::vector<Arc<T>*>* out_arcs = &source_node->outgoingArcs;

				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				this->removeArc(arc, &arc->getSourceNode()->outgoingArcs);
				this->removeArc(arc, &arc->getTargetNode()->incomingArcs);

				// Removes the arc from the global arcs index
				this->arcs.erase(arc->getId());
				// Deletes the arc
				delete arc;
		}

		/**
		 * Removes the node that correspond to the specified id from this
		 * graph.
		 *
		 * Incoming and outgoing arcs associated to this node are also deleted
		 * and unlinked from the corresponding source and target nodes.
		 *
		 * @param node_id id of the node to delete
		 */
		template<class T> void Graph<T>::removeNode(unsigned long node_id) {
			Node<T>* node_to_remove = nodes.at(node_id);

			// Deletes incoming arcs
			for(auto arc : node_to_remove->getIncomingArcs()) {
				this->unlink(arc);
				/*
				Node<T>* source_node = arc->getSourceNode();
				std::vector<Arc<T>*>* out_arcs = &source_node->outgoingArcs;

				// Removes the incoming arcs from the outgoingArcs list of the
				// associated source node
				for(auto it = out_arcs->begin(); it != out_arcs->end();) {
					if(*it == arc) {
						out_arcs->erase(it);
						it = out_arcs->end();
					}
					else {
						it++;
					}
				}
				// Removes the arc from the global arcs index
				this->arcs.erase(arc->getId());
				// Deletes the arc
				delete arc;
				*/
			}

			// Deletes outgoing arcs
			for(auto arc : node_to_remove->getOutgoingArcs()) {
				this->unlink(arc);
				/*
				Node<T>* target_node = arc->getTargetNode();
				std::vector<Arc<T>*>* in_arcs = &target_node->incomingArcs;

				// Removes the outgoing arcs from the incomingArcs list of the
				// associated target node
				for(auto it = in_arcs->begin(); it != in_arcs->end();) {
					if(*it == arc) {
						in_arcs->erase(it);
						it = in_arcs->end();
					}
					else {
						it++;
					}
				}
				// Removes the arc from the global arcs index
				this->arcs.erase(arc->getId());
				// Deletes the arc
				delete arc;
				*/
			}
			// Removes the node from the global nodes index
			nodes.erase(node_id);
			// Deletes the node
			delete node_to_remove;
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
