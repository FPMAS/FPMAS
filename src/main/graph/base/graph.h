#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <iostream>
#include "utils/macros.h"
#include "serializers.h"

#include <nlohmann/json.hpp>

#include "node.h"
#include "arc.h"
#include "layer.h"
#include "fossil.h"
#include "mpi.h"

namespace FPMAS {
	/**
	 * The FPMAS::graph namespace contains anything related to the graph
	 * structure.
	 */
	namespace graph {
		/**
		 * The FPMAS::graph namespace contains Graph, Node and Arc definitions.
		 */
		namespace base {
			template<NODE_PARAMS> class Node;
			template<NODE_PARAMS> class Arc;
			template<NODE_PARAMS> class Graph;

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
			 *  #include "graph.h"
			 *  #include <nlohmann/json.hpp>
			 *
			 *  using FPMAS::graph::Graph;
			 *  using FPMAS::graph::Node;
			 *
			 *  int main() {
			 *  	Graph<int> g;
			 *		Node<int>* n1 = g.buildNode(0ul, 0);
			 *		Node<int>* n2 = g.buildNode(1ul, 1);
			 *		g.link(n1, n2, 0ul);
			 *
			 *		json j = g;
			 *
			 *		std::out << j.dump() << std::endl;
			 *	}
			 *	```
			 *  Will print the following json :
			 *	```
			 *	{"arcs":[{"id":0,"link":["0","1"]}],"nodes":[{"data":0,"id":0},{"data":1,"id":1}]}
			 *	```
			 *  For more details about how json format used, see the
			 *  FPMAS::graph::to_json(json&, const Graph<NODE_PARAMS_SPEC>&) documentation.
			 *
			 *  Graph can also be un-serialized from json files with the same
			 *  format.
			 *
			 *  The following example code will build the graph serialized in the
			 *  previous example :
			 *	```
			 *	#include <iostream>
			 *	#include "graph.h"
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
			 * and the FPMAS::graph::from_json(const json&, Graph<NODE_PARAMS_SPEC>&)
			* function.
				*
				* @tparam T associated data type
				*/
				template<
					class T,
					typename LayerType = DefaultLayer,
					int N = 1
					>
				class Graph {
					friend Graph<NODE_PARAMS_SPEC> nlohmann::adl_serializer<Graph<NODE_PARAMS_SPEC>>::from_json(const json&);
					private:
					std::unordered_map<unsigned long, NODE*> nodes;
					std::unordered_map<unsigned long, ARC*> arcs;
					void removeArc(ARC*, std::vector<ARC*>*);

					public:
					NODE* getNode(unsigned long);
					const std::unordered_map<unsigned long, NODE*>& getNodes() const;

					ARC* getArc(unsigned long);
					const std::unordered_map<unsigned long, ARC*>& getArcs() const;

					NODE* buildNode(unsigned long);
					NODE* buildNode(unsigned long id, T data);
					NODE* buildNode(unsigned long id, float weight, T data);

					ARC* link(NODE*, NODE*, unsigned long);
					ARC* link(NODE*, NODE*, unsigned long, LayerType);
					ARC* link(unsigned long, unsigned long, unsigned long);
					ARC* link(unsigned long, unsigned long, unsigned long, LayerType);

					bool unlink(unsigned long);
					bool unlink(ARC*);
					FossilArcs<ARC> removeNode(unsigned long);

					~Graph();

				};

			/**
			 * Finds and returns the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return pointer to associated node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::getNode(unsigned long id) {
				return this->nodes.find(id)->second;
			}

			/**
			 * Returns a const reference to the nodes map of this graph.
			 *
			 * @return nodes contained in this graph
			 */
			template<NODE_PARAMS> const std::unordered_map<unsigned long, NODE*>& Graph<NODE_PARAMS_SPEC>::getNodes() const {
				return this->nodes;
			}

			/**
			 * Finds and returns the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return pointer to associated arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::getArc(unsigned long id) {
				return this->arcs.find(id)->second;
			}

			/**
			 * Returns a const reference to the arcs map of this graph.
			 *
			 * @return arcs contained in this graph
			 */
			template<NODE_PARAMS> const std::unordered_map<unsigned long, ARC*>& Graph<NODE_PARAMS_SPEC>::getArcs() const {
				return this->arcs;
			}

			/**
			 * Builds a node with the specified id, with a default weight and no
			 * data.
			 *
			 * @param id node id
			 * @return built node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id) {
				NODE* node = new NODE(id);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id and data, and adds it to this graph,
			 * and finally returns the built node.
			 *
			 * @param id node id
			 * @param data node's data
			 * @return pointer to built node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, T data) {
				NODE* node = new Node(id, data);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id weight and data, and adds it to this graph,
			 * and finally returns the built node.
			 *
			 * @param id node's id
			 * @param weight node's weight
			 * @param data node's data
			 * @return pointer to build node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, float weight, T data) {
				NODE* node = new Node(id, weight, data);
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
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(NODE *source, NODE *target, unsigned long arc_id, LayerType layer) {
				ARC* arc = new ARC(arc_id, source, target, layer);
				this->arcs[arc_id] = arc;
				return arc;
			}

			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(NODE *source, NODE *target, unsigned long arc_id){
				return link(source, target, arc_id, LayerType(0));
			}

			/**
			 * Same as link(NODE*, NODE*, unsigned long), but uses node ids
			 * instead to retrieve source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @return built arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(unsigned long source_id, unsigned long target_id, unsigned long arc_id, LayerType layer) {
				return link(this->getNode(source_id), this->getNode(target_id), arc_id);
			}

			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(unsigned long source_id, unsigned long target_id, unsigned long arc_id) {
				return link(source_id, target_id, arc_id, LayerType(0));
			}

			/**
			 * Convenient function to remove an arc from an incoming or outgoing
			 * arc list.
			 */
			template<NODE_PARAMS> void Graph<NODE_PARAMS_SPEC>::removeArc(ARC* arc, std::vector<ARC*>* arcList) {
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
			template<NODE_PARAMS> bool Graph<NODE_PARAMS_SPEC>::unlink(ARC* arc) {
				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				this->removeArc(arc, &arc->getSourceNode()->layer(arc->layer).outgoingArcs);
				this->removeArc(arc, &arc->getTargetNode()->layer(arc->layer).incomingArcs);

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
			 * Same as unlink(ARC*), but retrieving the arc from its ID.
			 *
			 * @param arcId id of the arc to delete
			 * @return true iff the arc has been deleted (i.e. it belongs to the
			 * graph)
			 */
			template<NODE_PARAMS> bool Graph<NODE_PARAMS_SPEC>::unlink(unsigned long arcId) {
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
			template<NODE_PARAMS> FossilArcs<ARC> Graph<NODE_PARAMS_SPEC>::removeNode(unsigned long node_id) {
				NODE* node_to_remove = nodes.at(node_id);

				FossilArcs<ARC> fossil;
				// Deletes incoming arcs
				for(auto arc : node_to_remove->getIncomingArcs()) {
					if(!this->unlink(arc))
						fossil.incomingArcs.insert(arc);
				}

				// Deletes outgoing arcs
				for(auto arc : node_to_remove->getOutgoingArcs()) {
					if(!this->unlink(arc))
						fossil.outgoingArcs.insert(arc);
				}

				// Removes the node from the global nodes index
				nodes.erase(node_id);
				// Deletes the node
				delete node_to_remove;

				return fossil;
			}

			/**
			 * Graph destructor.
			 *
			 * Deletes nodes and arcs remaining in this graph.
			 */
			template<NODE_PARAMS> Graph<NODE_PARAMS_SPEC>::~Graph() {
				for(auto node : this->nodes) {
					delete node.second;
				}
				for(auto arc : this->arcs) {
					delete arc.second;
				}
			}
		}
	}
}

#endif
