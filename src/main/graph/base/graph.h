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
			template<typename, typename, int N> class Node;
			template<typename, typename, int N> class Arc;

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
			template<typename T, typename IdType, int N = 1>
			class Graph {
				friend Graph<T, IdType, N> nlohmann::adl_serializer<Graph<T, IdType, N>>::from_json(const json&);
				private:
					std::unordered_map<IdType, Node<T, IdType, N>*> nodes;
					std::unordered_map<IdType, Arc<T, IdType, N>*> arcs;
					void removeArc(Arc<T, IdType, N>*, std::vector<Arc<T, IdType, N>*>*);

				public:
					Node<T, IdType, N>* getNode(IdType);
					const Node<T, IdType, N>* getNode(IdType) const;
					const std::unordered_map<IdType, Node<T, IdType, N>*>& getNodes() const;

					Arc<T, IdType, N>* getArc(IdType);
					const Arc<T, IdType, N>* getArc(IdType) const;
					const std::unordered_map<IdType, Arc<T, IdType, N>*>& getArcs() const;

					Node<T, IdType, N>* buildNode(IdType);
					Node<T, IdType, N>* buildNode(IdType id, T&& data);
					Node<T, IdType, N>* buildNode(IdType id, float weight, T&& data);

					Arc<T, IdType, N>* link(Node<T, IdType, N>*, Node<T, IdType, N>*, IdType);
					Arc<T, IdType, N>* link(Node<T, IdType, N>*, Node<T, IdType, N>*, IdType, LayerId);
					Arc<T, IdType, N>* link(IdType, IdType, IdType);
					Arc<T, IdType, N>* link(IdType, IdType, IdType, LayerId);

					void unlink(IdType);
					virtual void unlink(Arc<T, IdType, N>*);
					void removeNode(IdType);

					~Graph();

				};

			/**
			 * Returns a pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return pointer to associated node
			 */
			template<typename T, typename IdType, int N> Node<T, IdType, N>* Graph<T, IdType, N>::getNode(IdType id) {
				return this->nodes.at(id);
			}

			/**
			 * Returns a const pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return const pointer to associated node
			 */
			template<typename T, typename IdType, int N> const Node<T, IdType, N>* Graph<T, IdType, N>::getNode(IdType id) const {
				return this->nodes.at(id);
			}
			/**
			 * Returns a const reference to the nodes map of this graph.
			 *
			 * @return nodes contained in this graph
			 */
			template<typename T, typename IdType, int N> const std::unordered_map<IdType, Node<T, IdType, N>*>& Graph<T, IdType, N>::getNodes() const {
				return this->nodes;
			}

			/**
			 * Returns a pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return pointer to associated arc
			 */
			template<typename T, typename IdType, int N> Arc<T, IdType, N>* Graph<T, IdType, N>::getArc(IdType id) {
				return this->arcs.at(id);
			}

			/**
			 * Returns a const pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return const pointer to associated arc
			 */
			template<typename T, typename IdType, int N> const Arc<T, IdType, N>* Graph<T, IdType, N>::getArc(IdType id) const {
				return this->arcs.at(id);
			}

			/**
			 * Returns a const reference to the arcs map of this graph.
			 *
			 * @return arcs contained in this graph
			 */
			template<typename T, typename IdType, int N> const std::unordered_map<IdType, Arc<T, IdType, N>*>& Graph<T, IdType, N>::getArcs() const {
				return this->arcs;
			}

			/**
			 * Builds a node with the specified id, a default weight and no
			 * data, adds it to this graph, and finally returns the built node.
			 *
			 * @param id node id
			 * @return pointer to built node
			 */
			template<typename T, typename IdType, int N> Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType id) {
				Node<T, IdType, N>* node = new Node<T, IdType, N>(id);
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
			template<typename T, typename IdType, int N> Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType id, T&& data) {
				Node<T, IdType, N>* node = new Node<T, IdType, N>(id, std::forward<T>(data));
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
			template<typename T, typename IdType, int N> Node<T, IdType, N>* Graph<T, IdType, N>::buildNode(IdType id, float weight, T&& data) {
				Node<T, IdType, N>* node = new Node<T, IdType, N>(id, weight, std::forward<T>(data));
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
			template<typename T, typename IdType, int N> Arc<T, IdType, N>* Graph<T, IdType, N>::link(Node<T, IdType, N> *source, Node<T, IdType, N> *target, IdType arc_id, LayerId layer) {
				Arc<T, IdType, N>* arc = new Arc<T, IdType, N>(arc_id, source, target, layer);
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
			template<typename T, typename IdType, int N> Arc<T, IdType, N>* Graph<T, IdType, N>::link(Node<T, IdType, N> *source, Node<T, IdType, N> *target, IdType arc_id){
				return link(source, target, arc_id, DefaultLayer);
			}

			/**
			 * Same as link(Node<T, IdType, N>*, Node<T, IdType, N>*, IdType, LayerType), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @param layer layer on which the arc should be built
			 * @return built arc
			 */
			template<typename T, typename IdType, int N> Arc<T, IdType, N>* Graph<T, IdType, N>::link(IdType source_id, IdType target_id, IdType arc_id, LayerId layer) {
				return link(this->getNode(source_id), this->getNode(target_id), arc_id, layer);
			}

			/**
			 * Same as link(Node<T, IdType, N>*, Node<T, IdType, N>*, IdType), but uses node ids
			 * instead to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param arc_id new arc id
			 * @return built arc
			 */
			template<typename T, typename IdType, int N> Arc<T, IdType, N>* Graph<T, IdType, N>::link(IdType source_id, IdType target_id, IdType arc_id) {
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
			template<typename T, typename IdType, int N> void Graph<T, IdType, N>::unlink(Arc<T, IdType, N>* arc) {
				if(this->arcs.count(arc->getId()) == 0)
					throw exceptions::arc_out_of_graph(arc->getId());

				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				arc->unlink();

				this->arcs.erase(arc->getId());
				delete arc;
			}

			/**
			 * Same as unlink(Arc<T, IdType, N>*), but gets the arc from its ID.
			 *
			 * @param arcId id of the arc to delete
			 * @return true iff the arc has been deleted (i.e. it belongs to the
			 * graph)
			 */
			template<typename T, typename IdType, int N> void Graph<T, IdType, N>::unlink(IdType arcId) {
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
			template<typename T, typename IdType, int N> void Graph<T, IdType, N>::removeNode(IdType nodeId) {
				FPMAS_LOGD(-1, "GRAPH", "Removing node %lu", nodeId);
				Node<T, IdType, N>* node_to_remove;
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
						} catch (exceptions::arc_out_of_graph<IdType> e) {
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
						} catch (exceptions::arc_out_of_graph<IdType> e) {
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
			template<typename T, typename IdType, int N> Graph<T, IdType, N>::~Graph() {
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
