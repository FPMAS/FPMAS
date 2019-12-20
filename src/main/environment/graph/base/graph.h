#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "serializers.h"

#include <nlohmann/json.hpp>

#include "node.h"
#include "arc.h"
#include "fossil.h"
#include "mpi.h"

namespace FPMAS {
	/**
	 * The FPMAS::graph namespace contains Graph, Node and Arc definitions.
	 */
	namespace graph {
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
		 *		Node<int>* n1 = g.buildNode(0ul, new int(0));
		 *		Node<int>* n2 = g.buildNode(1ul, new int(1));
		 *		g.link(n1, n2, 0ul);
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
		 *	{"arcs":[{"id":0,"link":["0","1"]}],"nodes":[{"data":0,"id":0},{"data":1,"id":1}]}
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
		 *				{"id":0,"link":["0","1"]}
		 *				],
		 *			"nodes":[
		 *				{"data":0,"id":0},
		 *				{"data":1,"id":1}
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

				Node<T>* buildNode(unsigned long);
				Node<T>* buildNode(unsigned long id, T* data);
				Node<T>* buildNode(unsigned long id, float weight, T* data);

				Arc<T>* link(Node<T>* source, Node<T>* target, unsigned long arcLabel);
				Arc<T>* link(unsigned long source_id, unsigned long target_id, unsigned long arcLabel);

				bool unlink(unsigned long);
				bool unlink(Arc<T>*);
				FossilArcs<T> removeNode(unsigned long);

				~Graph();

		};

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
		 * Builds a node with the specified id, with a default weight and no
		 * data.
		 *
		 * @param id node id
		 * @return built node
		 */
		template<class T> Node<T>* Graph<T>::buildNode(unsigned long id) {
			Node<T>* node = new Node<T>(id);
			this->nodes[id] = node;
			return node;
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

		/**
		 * Convenient function to remove an arc from an incoming or outgoing
		 * arc list.
		 */
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

		/**
		 * Removes the specified arc from the graph and the incoming/outgoing
		 * arc lists of its target/source nodes, and finally `deletes` it.
		 *
		 * If the arc does not belong to the standard Graph arc register (e.g.
		 * : it is a GhostArc of a DistributedGraph instance), it is not
		 * `deleted` and `false` is returned.
		 *
		 * @param arc pointer to the arc to delete
		 * @return true iff the arc has been deleted (i.e. it belongs to the
		 * graph)
		 */
		template<class T> bool Graph<T>::unlink(Arc<T>* arc) {
				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				this->removeArc(arc, &arc->getSourceNode()->outgoingArcs);
				this->removeArc(arc, &arc->getTargetNode()->incomingArcs);

				// Removes the arc from the global arcs index
				if(this->arcs.erase(arc->getId()) > 0) {
					// Deletes the arc only if it was in the standard arcs set.
					// Arcs can actually have a different nature not handled by
					// this base class (e.g. : ghost arcs), and in consequence
					// such special arcs must be managed by corresponding
					// sub-classes (e.g. : DistributedGraph).
					delete arc;
					return true;
				}
				return false;
		}

		/**
		 * Same as unlink(Arc<T>*), but retrieving the arc from its ID.
		 *
		 * @param arcId id of the arc to delete
		 * @return true iff the arc has been deleted (i.e. it belongs to the
		 * graph)
		 */
		template<class T> bool Graph<T>::unlink(unsigned long arcId) {
			return this->unlink(this->arcs.at(arcId));
		}


		/**
		 * Removes the node that correspond to the specified id from this
		 * graph.
		 *
		 * Incoming and outgoing arcs associated to this node are also
		 * `deleted`
		 * and unlinked from the corresponding source and target nodes.
		 *
		 * The returned fossil contains the fossil arcs collected
		 * unlinking the node's arcs.
		 *
		 * @param node_id id of the node to delete
		 * @return collected fossil arcs
		 */
		template<class T> FossilArcs<T> Graph<T>::removeNode(unsigned long node_id) {
			Node<T>* node_to_remove = nodes.at(node_id);

			int rank;
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);
			FossilArcs<T> fossil;
			// Deletes incoming arcs
			for(auto arc : node_to_remove->getIncomingArcs()) {
				std::cout << rank << " - Node " << node_id << " in : " << arc->getId() << ", " << arc->getSourceNode()->getId() << std::endl;
				if(!this->unlink(arc))
					fossil.incomingArcs.insert(arc);
			}

			// Deletes outgoing arcs
			for(auto arc : node_to_remove->getOutgoingArcs()) {
				std::cout << rank << " - Node " << node_id << " out : "  << arc->getId() << ", " << arc->getTargetNode()->getId() << std::endl;
				if(!this->unlink(arc))
					fossil.outgoingArcs.insert(arc);
			}
			for(auto arc : fossil.incomingArcs)
				if(this->nodes.count(arc->getSourceNode()->getId()) > 0)
					std::cout << rank << " - ERROR in : " << arc->getId() << ", " << arc->getSourceNode()->getId() << std::endl;
			for(auto arc : fossil.outgoingArcs)
				if(this->nodes.count(arc->getTargetNode()->getId()) > 0)
					std::cout << rank << " - ERROR out : " << arc->getId() << ", " << arc->getTargetNode()->getId() << std::endl;
			// Removes the node from the global nodes index
			nodes.erase(node_id);
			// Deletes the node
			std::cout << "Delete " << node_id << std::endl;
			delete node_to_remove;

			return fossil;
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
