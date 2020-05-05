#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <iostream>
#include "utils/macros.h"
#include "default_id.h"
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
			template<typename, typename> class Node;
			template<typename, typename> class Arc;

			/**
			 * Layered Graph class.
			 *
			 * A layered Graph is a graph where each Node can simultaneously
			 * belong to several layers, so that each layer can be explored
			 * independently from each node.
			 *
			 *
			 * Because the layer count is provided at compile time as a
			 * template argument, managing the several layers can be done in
			 * constant time whatever the numbe of layers is. Each layer
			 * manages its own lists of _incoming_ and _outgoing_ arcs for each
			 * node.
			 *
			 * Layers can be accessed thanks to their id `i`, with `0<=i<N`
			 * through the Node::layer() function.
			 *
			 * The `buildNode` functions can be used to create nodes. Notice
			 * that data is provided as an `rvalue` reference. Two arguments
			 * justify this choice :
			 * 	1. This ensures that data instances that are MoveConstructible
			 * 	and MoveAssignable but not either CopyConstructible or
			 * 	CopyAssignable, such as std::unique_ptr instances, can be used
			 * 	in the Graph.
			 * 	2. Rather than being _copied to_ the Nodes, data is actually
			 * 	_moved to_ the nodes, so that the lifetime of the provided data
			 * 	becomes equal to the one of its containing Node.

			 *
			 * @tparam T associated data type
			 * @tparam IdType type of Nodes and Arcs ids
			 */
			template<typename T, typename IdType=DefaultId>
			class Graph {
				typedef Arc<T, IdType>* arc_ptr;
				typedef Node<T, IdType>* node_ptr;
				friend Arc<T, IdType>;
				friend Node<T, IdType>;

				private:
					std::unordered_map<IdType, node_ptr> nodes;
					std::unordered_map<IdType, arc_ptr> arcs;
					void removeArc(arc_ptr, std::vector<arc_ptr>*);

				protected:
					/**
					 * Current Node Id.
					 */
					IdType currentNodeId;
					/**
					 * Current Arc Id.
					 */
					IdType currentArcId;
					/**
					 * Protected default constructor, that might be used to
					 * initialize currentNodeId and currentArcId from the
					 * constructor body.
					 */
					Graph() {}

					node_ptr _buildNode(IdType id, float weight, T&& data);
					arc_ptr _link(IdType, IdType, IdType, LayerId);
					arc_ptr _link(IdType, node_ptr, node_ptr, LayerId);

				public:
					/**
					 * Graph constructor.
					 *
					 * Initializes currentNodeId and currentArcId with the
					 * provided initId.
					 */
					Graph(IdType initId) : currentNodeId(initId), currentArcId(initId) {}

					// Node getters
					node_ptr getNode(IdType);
					const Node<T, IdType>* getNode(IdType) const;
					const std::unordered_map<IdType, node_ptr>& getNodes() const;

					// Arc getters
					arc_ptr getArc(IdType);
					const Arc<T, IdType>* getArc(IdType) const;
					const std::unordered_map<IdType, arc_ptr>& getArcs() const;

					// Node constructors
					node_ptr buildNode();
					node_ptr buildNode(T&& data);
					node_ptr buildNode(float weight, T&& data);

					// Node destructor
					void removeNode(IdType);

					// Arc constructors
					arc_ptr link(node_ptr, node_ptr);
					virtual arc_ptr link(node_ptr, node_ptr, LayerId);
					arc_ptr link(IdType, IdType);
					virtual arc_ptr link(IdType, IdType, LayerId);

					// Arc destructors
					virtual void unlink(IdType);
					virtual void unlink(arc_ptr);

					// Graph destructors
					~Graph();

				};

			/**
			 * Returns a pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return pointer to associated node
			 *
			 * @throw exceptions::node_out_of_graph if the specified id does
			 * not correspond to any node
			 */
			template<typename T, typename IdType> Node<T, IdType>* Graph<T, IdType>::getNode(IdType id) {
				try {
					return this->nodes.at(id);
				} catch(std::out_of_range) {
					throw exceptions::node_out_of_graph<IdType>(id);
				}
			}

			/**
			 * Returns a const pointer to the node associated to the specified id within this
			 * graph.
			 *
			 * @param id node id
			 * @return const pointer to associated node
			 *
			 * @throw exceptions::node_out_of_graph if the specified id does
			 * not correspond to any node
			 */
			template<typename T, typename IdType> const Node<T, IdType>* Graph<T, IdType>::getNode(IdType id) const {
				try {
					return this->nodes.at(id);
				} catch(std::out_of_range) {
					throw exceptions::node_out_of_graph<IdType>(id);
				}
			}
			/**
			 * Returns a const reference to the nodes map of this graph.
			 *
			 * @return nodes contained in this graph
			 */
			template<typename T, typename IdType> const std::unordered_map<IdType, Node<T, IdType>*>& Graph<T, IdType>::getNodes() const {
				return this->nodes;
			}

			/**
			 * Returns a pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return pointer to associated arc
			 *
			 * @throw exceptions::arc_out_of_graph if the specified id does
			 * not correspond to any arc
			 */
			template<typename T, typename IdType> Arc<T, IdType>* Graph<T, IdType>::getArc(IdType id) {
				try {
					return this->arcs.at(id);
				} catch (std::out_of_range) {
					throw exceptions::arc_out_of_graph<IdType>(id);
				}
			}

			/**
			 * Returns a const pointer to the arc associated to the specified id within this
			 * graph.
			 *
			 * @param id arc id
			 * @return const pointer to associated arc
			 */
			template<typename T, typename IdType> const Arc<T, IdType>* Graph<T, IdType>::getArc(IdType id) const {
				try {
					return this->arcs.at(id);
				} catch (std::out_of_range) {
					throw exceptions::arc_out_of_graph<IdType>(id);
				}
			}

			/**
			 * Returns a const reference to the arcs map of this graph.
			 *
			 * @return arcs contained in this graph
			 */
			template<typename T, typename IdType> const std::unordered_map<IdType, Arc<T, IdType>*>& Graph<T, IdType>::getArcs() const {
				return this->arcs;
			}

			/**
			 * Builds a node with the specified id, weight and data.
			 *
			 * This protected function is only intended to be used internally, since
			 * final users are not supposed to manually provide Node ids.
			 *
			 * @param id node id
			 * @param weight node weight
			 * @param data rvalue reference to node data
			 *
			 * @see buildNode(T&&)
			 * @see buildNode(weight, T&&)
			 */
			template<typename T, typename IdType> Node<T, IdType>*
				Graph<T, IdType>::_buildNode(IdType id, float weight, T&& data) {
				node_ptr node = new Node<T, IdType>(
						id, weight, std::forward<T>(data)
						);
				this->nodes[id] = node;
				return node;
			}

			/**
			 * Builds a node with a default weight and no data, adds it to this
			 * graph, and finally returns the built node.
			 *
			 * @return pointer to built node
			 */
			template<typename T, typename IdType> Node<T, IdType>*
				Graph<T, IdType>::buildNode() {
					node_ptr node = new Node<T, IdType>(currentNodeId++);
					this->nodes[node->getId()] = node;
					return node;
				}

			/**
			 * Builds a node with the specified data, adds it to this graph,
			 * and finally returns the built node.
			 *
			 * @param data node's data
			 * @return pointer to built node
			 */
			template<typename T, typename IdType> Node<T, IdType>* Graph<T, IdType>::buildNode(T&& data) {
				node_ptr node = new Node<T, IdType>(currentNodeId++, std::forward<T>(data));
				this->nodes[node->getId()] = node;
				return node;
			}

			/**
			 * Builds a node with the specify id, weight and data, adds it to this graph,
			 * and finally returns a pointer to the built node.
			 *
			 * @param weight node's weight
			 * @param data rvalue reference to node's data
			 * @return pointer to build node
			 */
			template<typename T, typename IdType> Node<T, IdType>* Graph<T, IdType>::buildNode(float weight, T&& data) {
				node_ptr node = new Node<T, IdType>(
						currentNodeId++, weight, std::forward<T>(data)
						);
				this->nodes[node->getId()] = node;
				return node;
			}

			/**
			 * Links the specified nodes on the given layer with an arc with
			 * the specified arcId.
			 *
			 * This protected function is only intended to be used internally, since
			 * final users are not supposed to manually provide Arc ids.
			 *
			 * The provided layer should be such that `0<=layer<N`, behavior is
			 * unexpected otherwise.
			 *
			 * @param arcId arc's id
			 * @param source pointer to source node
			 * @param target pointer to target node
			 * @param layer layer id
			 *
			 * @see link(node_ptr, node_ptr)
			 * @see link(node_ptr, node_ptr, LayerId)
			 * @see link(IdType, IdType)
			 * @see link(IdType, IdType, LayerId)
			 */
			// TODO: exception if nodes does not belong to this graph
			// TODO: exception if bad layer
			template<typename T, typename IdType> Arc<T, IdType>*
				Graph<T, IdType>::_link(
					IdType arcId, node_ptr source, node_ptr target, LayerId layer
					) {
				arc_ptr arc = new Arc<T, IdType>(
						arcId, source, target, layer
						);
				this->arcs[arcId] = arc;
				return arc;

			}

			/**
			 * Links the specified nodes on the given layer with an arc with
			 * the specified arcId.
			 *
			 * This protected function is only intended to be used internally, since
			 * final users are not supposed to manually provide Arc ids.
			 *
			 * The provided layer should be such that `0<=layer<N`, behavior is
			 * unexpected otherwise.
			 *
			 * @param arcId arc's id
			 * @param source source node's id 
			 * @param target target node's id
			 * @param layer layer id
			 * @throw exceptions::node_out_of_graph iff at least one of the two
			 * node ids does not correspond to a Node in this Graph
			 *
			 * @see _link(IdType, node_ptr, node_ptr, LayerId)
			 * @see link(node_ptr, node_ptr)
			 * @see link(node_ptr, node_ptr, LayerId)
			 * @see link(IdType, IdType)
			 * @see link(IdType, IdType, LayerId)
			 */
			template<typename T, typename IdType> Arc<T, IdType>*
				Graph<T, IdType>::_link(
					IdType arcId, IdType source, IdType target, LayerId layer
					) {
					try {
						return this->_link(
								arcId, this->getNode(source), this->getNode(target), layer
								);
					} catch(exceptions::node_out_of_graph<IdType>) {
						throw;
					}
			}

			/**
			 * Builds a directed arc in the specified layer from the source
			 * node to the target node, adds it to the graph and finally
			 * returns a pointer to the built arc.
			 *
			 * The provided layer should be such that `0<=layer<N`, behavior is
			 * unexpected otherwise.
			 *
			 * @param source pointer to source node
			 * @param target pointer to target node
			 * @param layer layer on which the arc should be built
			 * @return pointer to built arc
			 */
			template<typename T, typename IdType> Arc<T, IdType>*
				Graph<T, IdType>::link(node_ptr source, node_ptr target, LayerId layer) {
				return this->_link(currentArcId++, source, target, layer);
			}

			/**
			 * Builds a directed arc in the default layer from the source node
			 * to the target node, adds it to the graph and finally returns a
			 * pointer to the built arc.
			 *
			 * Calls the (possibly overriden) link(node_ptr, node_ptr, LayerId)
			 * function with `layerId=DefaultLayer`.
			 *
			 * @param source pointer to source node
			 * @param target pointer to target node
			 * @return pointer to built arc
			 */
			template<typename T, typename IdType> Arc<T, IdType>*
				Graph<T, IdType>::link(node_ptr source, node_ptr target){
				return this->link(source, target, DefaultLayer);
			}

			/**
			 * Same as link(node_ptr, node_ptr, LayerType), but uses node ids
			 * to get source and target nodes in the current graph.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @param layer layer on which the arc should be built
			 * @return built arc
			 *
			 * @throw exceptions::node_out_of_graph iff at least one of the two
			 * node ids does not correspond to a Node in this Graph
			 */
			template<typename T, typename IdType> Arc<T, IdType>*
				Graph<T, IdType>::link(
						IdType source_id, IdType target_id, LayerId layer
						) {
				try {
					return link(
							this->getNode(source_id), this->getNode(target_id), layer
							);
				} catch(exceptions::node_out_of_graph<IdType>) {
					throw;
				}
			}

			/**
			 * Same as link(node_ptr, node_ptr), but uses node ids to get
			 * source and target nodes in the current graph.
			 *
			 * Calls the (possibly overriden) link(IdType, IdType, LayerId)
			 * function with `layerId=DefaultLayer`.
			 *
			 * @param source_id source node id
			 * @param target_id target node id
			 * @return built arc
			 *
			 * @throw exceptions::node_out_of_graph iff at least one of the two
			 * node ids does not correspond to a Node in this Graph
			 */
			template<typename T, typename IdType> Arc<T, IdType>* Graph<T, IdType>::link(IdType source_id, IdType target_id) {
				try {
					return this->link(source_id, target_id, DefaultLayer);
				} catch(exceptions::node_out_of_graph<IdType>) {
					throw;
				}
			}

			/**
			 * Removes the specified arc from the graph and the incoming/outgoing
			 * arc lists of its target/source nodes, and finally `deletes` it.
			 *
			 * @param arc pointer to the arc to delete
			 * @throw exceptions::arc_out_of_graph iff the specified Arc does
			 * not belong to this Graph
			 */
			// TODO: improve to not use count
			template<typename T, typename IdType> void Graph<T, IdType>::unlink(arc_ptr arc) {
				if(this->arcs.count(arc->getId()) == 0)
					throw exceptions::arc_out_of_graph(arc->getId());

				// Removes the incoming arcs from the incoming/outgoing
				// arc lists of target/source nodes.
				arc->unlink();

				this->arcs.erase(arc->getId());
				delete arc;
			}

			/**
			 * Same as unlink(arc_ptr), but gets the arc from its id.
			 *
			 * @param arcId id of the arc to delete
			 * @throw exceptions::arc_out_of_graph iff the specified arcId does
			 * not correspond to any Arc in this Graph
			 */
			template<typename T, typename IdType> void Graph<T, IdType>::unlink(IdType arcId) {
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
			 *
			 * @throw exceptions::node_out_of_graph iff the specified id does
			 * not correspond to any Node in this Graph
			 */
			template<typename T, typename IdType> void Graph<T, IdType>::removeNode(IdType nodeId) {
				FPMAS_LOGD(-1, "GRAPH", "Removing node %s", ID_C_STR(nodeId));
				node_ptr node_to_remove;
				try {
					node_to_remove = nodes.at(nodeId);
				} catch(std::out_of_range) {
					throw exceptions::node_out_of_graph(nodeId);
				}

				// Deletes incoming arcs
				for(auto& layer : node_to_remove->layers) {
					for(auto arc : layer.second.getIncomingArcs()) {
						try {
							FPMAS_LOGV(
									-1,
									"GRAPH",
									"Unlink incoming arc %s",
									ID_C_STR(arc->getId())
									);
							this->unlink(arc);
						} catch (exceptions::arc_out_of_graph<IdType> e) {
							FPMAS_LOGE(-1, "GRAPH", "Error unlinking incoming arc : %s", e.what());
							throw;
						}
					}
					// Deletes outgoing arcs
					for(auto arc : layer.second.getOutgoingArcs()) {
						try {
							FPMAS_LOGV(
									-1,
									"GRAPH",
									"Unlink outgoing arc %s",
									ID_C_STR(arc->getId())
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
				FPMAS_LOGD(-1, "GRAPH", "Node %s removed.", ID_C_STR(nodeId));
			}

			/**
			 * Graph destructor.
			 *
			 * Deletes nodes and arcs remaining in this graph.
			 */
			template<typename T, typename IdType> Graph<T, IdType>::~Graph() {
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
