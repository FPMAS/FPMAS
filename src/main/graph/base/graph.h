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

#include "utils/log.h"
#include "exceptions.h"
#include "node.h"
#include "arc.h"
#include "layer.h"
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
			template<typename T, int N> class Node;
			template<typename T, int N> class Arc;
			template<typename T, int N> class Graph;

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
			 * Because the layer count is provided at compile time as a template
			 * argument, managing the several layers can be done in constant
			 * time whatever the numbe of layers is.
			 *
			 * @tparam T associated data type
			 * @tparam N layers count
			 */
			template<typename T, int N = 1>
			class Graph {
				friend Graph<T, N> nlohmann::adl_serializer<Graph<T, N>>::from_json(const json&);
				private:
					std::unordered_map<NodeId, Node<T, N>*> nodes;
					std::unordered_map<ArcId, Arc<T, N>*> arcs;
					void removeArc(Arc<T, N>*, std::vector<Arc<T, N>*>*);

				public:
					Node<T, N>* getNode(NodeId);
					const Node<T, N>* getNode(NodeId) const;
					const std::unordered_map<NodeId, Node<T, N>*>& getNodes() const;

					Arc<T, N>* getArc(ArcId);
					const Arc<T, N>* getArc(ArcId) const;
					const std::unordered_map<ArcId, Arc<T, N>*>& getArcs() const;

					Node<T, N>* buildNode(NodeId);
					Node<T, N>* buildNode(NodeId id, T&& data);
					Node<T, N>* buildNode(NodeId id, float weight, T&& data);

					Arc<T, N>* link(Node<T, N>*, Node<T, N>*, ArcId);
					Arc<T, N>* link(Node<T, N>*, Node<T, N>*, ArcId, LayerId);
					Arc<T, N>* link(NodeId, NodeId, ArcId);
					Arc<T, N>* link(NodeId, NodeId, ArcId, LayerId);

					void unlink(ArcId);
					virtual void unlink(Arc<T, N>*);
					void removeNode(NodeId);

					~Graph();

				};

			/**
			 * Returns a pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return pointer to associated node
			 */
			template<typename T, int N> Node<T, N>* Graph<T, N>::getNode(NodeId id) {
				return this->nodes.at(id);
			}

			/**
			 * Returns a const pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return const pointer to associated node
			 */
			template<typename T, int N> const Node<T, N>* Graph<T, N>::getNode(NodeId id) const {
				return this->nodes.at(id);
			}
			/**
			 * Returns a const reference to the nodes map of this graph.
			 *
			 * @return nodes contained in this graph
			 */
			template<typename T, int N> const std::unordered_map<NodeId, Node<T, N>*>& Graph<T, N>::getNodes() const {
				return this->nodes;
			}

			/**
			 * Returns a pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return pointer to associated arc
			 */
			template<typename T, int N> Arc<T, N>* Graph<T, N>::getArc(ArcId id) {
				return this->arcs.at(id);
			}

			/**
			 * Returns a const pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return const pointer to associated arc
			 */
			template<typename T, int N> const Arc<T, N>* Graph<T, N>::getArc(ArcId id) const {
				return this->arcs.at(id);
			}

			/**
			 * Returns a const reference to the arcs map of this graph.
			 *
			 * @return arcs contained in this graph
			 */
			template<typename T, int N> const std::unordered_map<ArcId, Arc<T, N>*>& Graph<T, N>::getArcs() const {
				return this->arcs;
			}

			/**
			 * Builds a node with the specified id, a default weight and no
			 * data, adds it to this graph, and finally returns the built node.
			 *
			 * @param id node id
			 * @return pointer to built node
			 */
			template<typename T, int N> Node<T, N>* Graph<T, N>::buildNode(NodeId id) {
				Node<T, N>* node = new Node<T, N>(id);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id and data, adds it to this graph,
			 * and finally returns the built node. This functions ensures that
			 * data instances that are MoveConstructible and MoveAssignable but not
			 * either CopyConstructible or CopyAssignable, such as std::unique_ptr
			 * instances, can be used in the DistributedGraph.
			 *
			 * @param id node id
			 * @param data node's data
			 * @return pointer to built node
			 */
			template<typename T, int N> Node<T, N>* Graph<T, N>::buildNode(NodeId id, T&& data) {
				Node<T, N>* node = new Node<T, N>(id, std::move(data));
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id, weight and data, adds it to this graph,
			 * and finally returns a pointer to the built node. This functions ensures that
			 * data instances that are MoveConstructible and MoveAssignable but not
			 * either CopyConstructible or CopyAssignable, such as std::unique_ptr
			 * instances, can be used in the DistributedGraph.
			 *
			 * @param id node's id
			 * @param weight node's weight
			 * @param data node's data
			 * @return pointer to build node
			 */
			template<typename T, int N> Node<T, N>* Graph<T, N>::buildNode(NodeId id, float weight, T&& data) {
				Node<T, N>* node = new Node<T, N>(id, weight, std::move(data));
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
			template<typename T, int N> Arc<T, N>* Graph<T, N>::link(Node<T, N> *source, Node<T, N> *target, ArcId arc_id, LayerId layer) {
				Arc<T, N>* arc = new Arc<T, N>(arc_id, source, target, layer);
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
			template<typename T, int N> Arc<T, N>* Graph<T, N>::link(Node<T, N> *source, Node<T, N> *target, ArcId arc_id){
				return link(source, target, arc_id, DefaultLayer);
			}

			/**
			 * Same as link(Node<T, N>*, Node<T, N>*, ArcId, LayerType), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @param layer layer on which the arc should be built
			 * @return built arc
			 */
			template<typename T, int N> Arc<T, N>* Graph<T, N>::link(NodeId source_id, NodeId target_id, ArcId arc_id, LayerId layer) {
				return link(this->getNode(source_id), this->getNode(target_id), arc_id, layer);
			}

			/**
			 * Same as link(Node<T, N>*, Node<T, N>*, ArcId), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @return built arc
			 */
			template<typename T, int N> Arc<T, N>* Graph<T, N>::link(NodeId source_id, NodeId target_id, ArcId arc_id) {
				return link(source_id, target_id, arc_id, DefaultLayer);
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
			template<typename T, int N> void Graph<T, N>::unlink(Arc<T, N>* arc) {
				if(this->arcs.count(arc->getId()) == 0)
					throw exceptions::arc_out_of_graph(arc->getId());

				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				arc->unlink();

				this->arcs.erase(arc->getId());
				delete arc;
			}

			/**
			 * Same as unlink(Arc<T, N>*), but gets the arc from its ID.
			 *
			 * @param arcId id of the arc to delete
			 * @return true iff the arc has been deleted (i.e. it belongs to the
			 * graph)
			 */
			template<typename T, int N> void Graph<T, N>::unlink(ArcId arcId) {
				try {
					this->unlink(this->arcs.at(arcId));
				} catch (std::out_of_range) {
					throw exceptions::arc_out_of_graph(arcId);
				}
			}


			/**
			 * Removes the Node that correspond to the specified id from this
			 * graph.
			 *
			 * Incoming and outgoing arcs associated to this node are also
			 * removed and unlinked from the corresponding source and target
			 * nodes.
			 *
			 * @param nodeId id of the node to delete
			 */
			template<typename T, int N> void Graph<T, N>::removeNode(NodeId nodeId) {
				FPMAS_LOGD(-1, "GRAPH", "Removing node %lu", nodeId);
				Node<T, N>* node_to_remove;
				try {
					node_to_remove = nodes.at(nodeId);
				} catch(std::out_of_range) {
					throw exceptions::node_out_of_graph(nodeId);
				}

				// Deletes incoming arcs
				for(auto& layer : node_to_remove->layers) {
					for(auto arc : layer.getIncomingArcs()) {
						try {
							FPMAS_LOGV(
									-1,
									"GRAPH",
									"Unlink incoming arc %lu",
									arc->getId()
									);
							this->unlink(arc);
						} catch (exceptions::arc_out_of_graph e) {
							FPMAS_LOGE(-1, "GRAPH", "Error unlinking incoming arc : %s", e.what());
							throw;
						}
					}
					// Deletes outgoing arcs
					for(auto arc : layer.getOutgoingArcs()) {
						try {
							FPMAS_LOGV(
									-1,
									"GRAPH",
									"Unlink outgoing arc %lu",
									arc->getId()
									);
							this->unlink(arc);
						} catch (exceptions::arc_out_of_graph e) {
							FPMAS_LOGE(-1, "GRAPH", "Error unlinking outgoing arc : %s", e.what());
							throw;
						}
					}
				}

				// Removes the node from the global nodes index
				nodes.erase(nodeId);
				// Deletes the node
				delete node_to_remove;
				FPMAS_LOGD(-1, "GRAPH", "Node %lu removed.", nodeId);
			}

			/**
			 * Graph destructor.
			 *
			 * Deletes nodes and arcs remaining in this graph.
			 */
			template<typename T, int N> Graph<T, N>::~Graph() {
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
