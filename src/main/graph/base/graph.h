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
			 * Layered Graph class.
			 *
			 * A layered Graph is a graph where each Node can simultaneously
			 * belong to several layers, so that each layer can be explored
			 * independently from each node.
			 *
			 * Layered are represented as predefined arc types defined in the
			 * LayerType enum parameter. Each node then manages lists of
			 * _incoming_ and _outgoing_ arcs for each type.
			 *
			 * Because layer types are provided at compile time as template
			 * parameters, managing the several layers can be done in constant
			 * time whatever the numbe of layers is.
			 *
			 * In order to be valid, the `LayerType` and `N` template
			 * parameters must follow some constraints :
			 * - `LayerType` must be an enum using integers as underlying type,
			 *   assigning positive values to each layer, starting from 0. The
			 *   associated index can be thought as the level of each layer.
			 *   Here is a valid example :
			 *   \code{.cpp}
			 *   enum ExampleLayer {
			 *   	DEFAULT = 0,
			 *   	COMMUNICATION = 1,
			 *   	ROAD_GRAPH = 2
			 *   }
			 *   \endcode
			 * - `N` must be equal to the number of layers defined by
			 *   `LayerType` (in the example, `N = 3`)
			 *
			 * @tparam T associated data type
			 * @tparam LayerType enum describing the available layers
			 * @tparam N number of layers
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
					const NODE* getNode(unsigned long) const;
					const std::unordered_map<unsigned long, NODE*>& getNodes() const;

					ARC* getArc(unsigned long);
					const ARC* getArc(unsigned long) const;
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
			 * Returns a pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return pointer to associated node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::getNode(unsigned long id) {
				return this->nodes.at(id);
			}

			/**
			 * Returns a const pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return const pointer to associated node
			 */
			template<NODE_PARAMS> const NODE* Graph<NODE_PARAMS_SPEC>::getNode(unsigned long id) const {
				return this->nodes.at(id);
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
			 * Returns a pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return pointer to associated arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::getArc(unsigned long id) {
				return this->arcs.at(id);
			}

			/**
			 * Returns a const pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return const pointer to associated arc
			 */
			template<NODE_PARAMS> const ARC* Graph<NODE_PARAMS_SPEC>::getArc(unsigned long id) const {
				return this->arcs.at(id);
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
			 * Builds a node with the specified id, a default weight and no
			 * data, adds it to this graph, and finally returns the built node.
			 *
			 * @param id node id
			 * @return pointer to built node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id) {
				NODE* node = new NODE(id);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id and data, adds it to this graph,
			 * and finally returns the built node.
			 *
			 * @param id node id
			 * @param data node's data
			 * @return pointer to built node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, T data) {
				NODE* node = new NODE(id, data);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id, weight and data, adds it to this graph,
			 * and finally returns a pointer to the built node.
			 *
			 * @param id node's id
			 * @param weight node's weight
			 * @param data node's data
			 * @return pointer to build node
			 */
			template<NODE_PARAMS> NODE* Graph<NODE_PARAMS_SPEC>::buildNode(unsigned long id, float weight, T data) {
				NODE* node = new NODE(id, weight, data);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a directed arc in the specified layer with the specified id from the source node to the
			 * target node, adds it to the graph and finally returns a pointer to the built arc.
			 *
			 * @param source pointer to source node
			 * @param target pointer to target node
			 * @param arc_id arc id
			 * @param layer layer on which the arc should be built
			 * @return pointer to built arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(NODE *source, NODE *target, unsigned long arc_id, LayerType layer) {
				ARC* arc = new ARC(arc_id, source, target, layer);
				this->arcs[arc_id] = arc;
				return arc;
			}

			/**
			 * Builds a directed arc in the default layer with the specified id from the source node to the
			 * target node, adds it to the graph and finally returns a pointer to the built arc.
			 *
			 * @param source pointer to source node
			 * @param target pointer to target node
			 * @param arc_id arc id
			 * @return pointer to built arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(NODE *source, NODE *target, unsigned long arc_id){
				return link(source, target, arc_id, LayerType(0));
			}

			/**
			 * Same as link(NODE*, NODE*, unsigned long, LayerType), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @param layer layer on which the arc should be built
			 * @return built arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(unsigned long source_id, unsigned long target_id, unsigned long arc_id, LayerType layer) {
				return link(this->getNode(source_id), this->getNode(target_id), arc_id, layer);
			}

			/**
			 * Same as link(NODE*, NODE*, unsigned long), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @return built arc
			 */
			template<NODE_PARAMS> ARC* Graph<NODE_PARAMS_SPEC>::link(unsigned long source_id, unsigned long target_id, unsigned long arc_id) {
				return link(source_id, target_id, arc_id, LayerType(0));
			}

			/*
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
			 * Same as unlink(ARC*), but gets the arc from its ID.
			 *
			 * @param arcId id of the arc to delete
			 * @return true iff the arc has been deleted (i.e. it belongs to the
			 * graph)
			 */
			template<NODE_PARAMS> bool Graph<NODE_PARAMS_SPEC>::unlink(unsigned long arcId) {
				return this->unlink(this->arcs.at(arcId));
			}


			/**
			 * Removes the Node that correspond to the specified id from this
			 * graph.
			 *
			 * Incoming and outgoing arcs associated to this node are also
			 * removed and unlinked from the corresponding source and target
			 * nodes.
			 *
			 * The returned fossil contains the fossil arcs collected unlinking
			 * the node's arcs.
			 *
			 * @param node_id id of the node to delete
			 * @return collected fossil arcs
			 */
			template<NODE_PARAMS> FossilArcs<ARC>
				Graph<NODE_PARAMS_SPEC>::removeNode(unsigned long node_id) {
					NODE* node_to_remove = nodes.at(node_id);

				FossilArcs<ARC> fossil;
				// Deletes incoming arcs
				for(auto layer : node_to_remove->layers) {
					for(auto arc : layer.getIncomingArcs()) {
						if(!this->unlink(arc))
							fossil.incomingArcs.insert(arc);
					}
				}

				// Deletes outgoing arcs
				for(auto layer : node_to_remove->layers) {
					for(auto arc : layer.getOutgoingArcs()) {
						if(!this->unlink(arc))
							fossil.outgoingArcs.insert(arc);
					}
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
