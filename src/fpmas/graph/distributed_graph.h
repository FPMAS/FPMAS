#ifndef FPMAS_DISTRIBUTED_GRAPH_H
#define FPMAS_DISTRIBUTED_GRAPH_H

/** \file src/fpmas/graph/distributed_graph.h
 * DistributedGraph implementation.
 */

#include <set>

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/graph/location_manager.h"
#include "fpmas/synchro/synchro.h"

#include "fpmas/graph/graph.h"

#define DIST_GRAPH_PARAMS\
	typename T,\
	template<typename> class SyncMode,\
	template<typename> class DistNodeImpl,\
	template<typename> class DistEdgeImpl,\
	template<typename> class TypedMpi,\
	template<typename> class LocationManagerImpl

#define DIST_GRAPH_PARAMS_SPEC\
	T,\
	SyncMode,\
	DistNodeImpl,\
	DistEdgeImpl,\
	TypedMpi,\
	LocationManagerImpl

namespace fpmas { namespace graph {

	/**
	 * Contains fpmas::api::graph implementation details.
	 */
	namespace detail {

		/**
		 * api::graph::DistributedGraph implementation.
		 */
		template<DIST_GRAPH_PARAMS>
			class DistributedGraph : 
				public Graph<
				api::graph::DistributedNode<T>,
				api::graph::DistributedEdge<T>>,
				public api::graph::DistributedGraph<T>
		{
			public:
				static_assert(
						std::is_base_of<api::graph::DistributedNode<T>, DistNodeImpl<T>>::value,
						"DistNodeImpl must implement api::graph::DistributedNode"
						);
				static_assert(
						std::is_base_of<api::graph::DistributedEdge<T>, DistEdgeImpl<T>>::value,
						"DistEdgeImpl must implement api::graph::DistributedEdge"
						);

			public:
				/**
				 * DistributedNode API.
				 */
				typedef api::graph::DistributedNode<T> NodeType;
				/**
				 * DistributedEdge API.
				 */
				typedef api::graph::DistributedEdge<T> EdgeType;

				using typename api::graph::DistributedGraph<T>::NodeMap;
				using typename api::graph::DistributedGraph<T>::SetLocalNodeCallback;
				using typename api::graph::DistributedGraph<T>::SetDistantNodeCallback;

				DistributedGraph(const DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&)
					= delete;
				DistributedGraph& operator=(
						const DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&)
					= delete;

				/**
				 * DistributedGraph move constructor.
				 */
				DistributedGraph(DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&& graph);
				/**
				 * DistributedGraph move assignment operator.
				 */
				DistributedGraph& operator=(DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&& graph);

			private:
				class EraseNodeCallback : public api::utils::Callback<NodeType*> {
					private:
						DistributedGraph<DIST_GRAPH_PARAMS_SPEC>& graph;
						bool enabled = true;
					public:
						EraseNodeCallback(DistributedGraph<DIST_GRAPH_PARAMS_SPEC>& graph)
							: graph(graph) {}

						void disable() {
							enabled = false;
						}
						void call(NodeType* node) {
							if(enabled) {
								graph.location_manager.remove(node);
								graph.unsynchronized_nodes.erase(node);
							}
						}
				};

				api::communication::MpiCommunicator* mpi_communicator;
				TypedMpi<DistributedId> id_mpi {*mpi_communicator};
				TypedMpi<std::pair<DistributedId, int>> location_mpi {*mpi_communicator};

				LocationManagerImpl<T> location_manager;
				SyncMode<T> sync_mode;

				std::vector<SetLocalNodeCallback*> set_local_callbacks;
				std::vector<SetDistantNodeCallback*> set_distant_callbacks;

				EraseNodeCallback* erase_node_callback;

				NodeType* _buildNode(NodeType*);

				void setLocal(
						api::graph::DistributedNode<T>* node,
						typename api::graph::SetLocalNodeEvent<T>::Context context
						);
				void setDistant(
						api::graph::DistributedNode<T>* node,
						typename api::graph::SetDistantNodeEvent<T>::Context context
						);

				void triggerSetLocalCallbacks(
						const api::graph::SetLocalNodeEvent<T>& event) {
					for(auto callback : set_local_callbacks)
						callback->call(event);
				}

				void triggerSetDistantCallbacks(
						const api::graph::SetDistantNodeEvent<T>& event) {
					for(auto callback : set_distant_callbacks)
						callback->call(event);
				}

				void clearDistantNodes();
				void clearNode(NodeType*);
				void _distribute(api::graph::PartitionMap);

				DistributedId node_id;
				DistributedId edge_id;

				std::unordered_set<api::graph::DistributedNode<T>*> unsynchronized_nodes;

			public:
				/**
				 * DistributedGraph constructor.
				 *
				 * @param comm MPI communicator
				 */
				DistributedGraph(api::communication::MpiCommunicator& comm) :
					mpi_communicator(&comm), location_manager(*mpi_communicator, id_mpi, location_mpi),
					sync_mode(*this, *mpi_communicator),
					// Graph base takes ownership of the erase_node_callback
					erase_node_callback(new EraseNodeCallback(*this)),
					node_id(mpi_communicator->getRank(), 0),
					edge_id(mpi_communicator->getRank(), 0) {
						this->addCallOnEraseNode(erase_node_callback);
					}

				/**
				 * Returns the internal MpiCommunicator.
				 *
				 * @return reference to the internal MpiCommunicator
				 */
				api::communication::MpiCommunicator& getMpiCommunicator() override {
					return *mpi_communicator;
				};

				/**
				 * \copydoc getMpiCommunicator()
				 */
				const api::communication::MpiCommunicator& getMpiCommunicator() const override {
					return *mpi_communicator;
				};

				DistributedId currentNodeId() const override {return node_id;}
				void setCurrentNodeId(DistributedId id) override {this->node_id = id;}
				DistributedId currentEdgeId() const override {return edge_id;}
				void setCurrentEdgeId(DistributedId id) override {this->edge_id = id;}

				NodeType* importNode(NodeType* node) override;
				EdgeType* importEdge(EdgeType* edge) override;
				std::unordered_set<api::graph::DistributedNode<T>*> getUnsyncNodes() const override {
					return unsynchronized_nodes;
				}


				/**
				 * @deprecated
				 * Use synchronizationMode() instead.
				 *
				 * Returns the internal SyncMode instance.
				 *
				 * @return sync mode
				 */
				const SyncMode<T>& getSyncMode() const {return sync_mode;}

				SyncMode<T>& synchronizationMode() override {return sync_mode;}

				const LocationManagerImpl<T>&
					getLocationManager() const override {return location_manager;}
				LocationManagerImpl<T>&
					getLocationManager() override {return location_manager;}

				void balance(api::graph::LoadBalancing<T>& load_balancing) override {
					balance(load_balancing, api::graph::PARTITION);
				};

				void balance(
						api::graph::LoadBalancing<T>& load_balancing,
						api::graph::PartitionMode partition_mode
						) override {
					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Balancing graph (%lu nodes, %lu edges)",
							this->getNodes().size(), this->getEdges().size());

					sync_mode.getSyncLinker().synchronize();
					this->_distribute(load_balancing.balance(
								this->location_manager.getLocalNodes(),
								partition_mode
								));

					// Data synchronization of newly imported DISTANT nodes.
					// In GhostMode, this import DISTANT nodes data, so that it can
					// be used directly following the _distribute() operation
					// without any additional and complete synchronize().
					this->synchronize(this->unsynchronized_nodes, false);

					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Graph balanced : %lu nodes, %lu edges",
							this->getNodes().size(), this->getEdges().size());
				}

				void balance(
						api::graph::FixedVerticesLoadBalancing<T>& load_balancing,
						api::graph::PartitionMap fixed_vertices
						) override {
					balance(load_balancing, fixed_vertices, api::graph::PARTITION);
				};

				void balance(
						api::graph::FixedVerticesLoadBalancing<T>& load_balancing,
						api::graph::PartitionMap fixed_vertices,
						api::graph::PartitionMode partition_mode
						) override {

					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Balancing graph (%lu nodes, %lu edges)",
							this->getNodes().size(), this->getEdges().size());

					sync_mode.getSyncLinker().synchronize();
					this->_distribute(load_balancing.balance(
								this->location_manager.getLocalNodes(),
								fixed_vertices,
								partition_mode));

					// Same as above
					this->synchronize(this->unsynchronized_nodes, false);

					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Graph balanced : %lu nodes, %lu edges",
							this->getNodes().size(), this->getEdges().size());
				}

				void distribute(api::graph::PartitionMap partition) override;

				void synchronize() override;
				void synchronize(
					std::unordered_set<NodeType*> nodes,
					bool synchronize_links = true
					) override;

				NodeType* buildNode(T&& = std::move(T())) override;
				NodeType* buildNode(const T&) override;
				api::graph::DistributedNode<T>* insertDistant(api::graph::DistributedNode<T>*) override;

				EdgeType* link(NodeType* const src, NodeType* const tgt, api::graph::LayerId layer) override;

				void removeNode(api::graph::DistributedNode<T>*) override;
				void removeNode(DistributedId id) override {
					this->removeNode(this->getNode(id));
				}

				void unlink(EdgeType*) override;
				void unlink(DistributedId id) override {
					this->unlink(this->getEdge(id));
				}

				void switchLayer(EdgeType* edge, api::graph::LayerId layer_id) override;

				void addCallOnSetLocal(SetLocalNodeCallback* callback) override {
					set_local_callbacks.push_back(callback);
				};
				std::vector<SetLocalNodeCallback*> onSetLocalCallbacks() const override {
					return set_local_callbacks;
				}

				void addCallOnSetDistant(SetDistantNodeCallback* callback) override {
					set_distant_callbacks.push_back(callback);
				};
				std::vector<SetDistantNodeCallback*> onSetDistantCallbacks() const override {
					return set_distant_callbacks;
				}

				~DistributedGraph();
		};

		template<DIST_GRAPH_PARAMS>
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::DistributedGraph(DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&& graph) :
				// Calls base Graph move constructor
				Graph<api::graph::DistributedNode<T>, api::graph::DistributedEdge<T>>(std::move(graph)),
				// Moves DistributedGraph specific fields
				mpi_communicator(graph.mpi_communicator),
				location_manager(std::move(graph.location_manager)),
				sync_mode(std::move(graph.sync_mode)),
				set_local_callbacks(std::move(graph.set_local_callbacks)),
				set_distant_callbacks(std::move(graph.set_distant_callbacks)),
				erase_node_callback(graph.erase_node_callback),
				node_id(std::move(graph.node_id)), edge_id(std::move(graph.edge_id)) {
					// TODO: Refactor callbacks system
					// Prevents erase_node_callback->disable() when `graph` is
					// deleted
					graph.erase_node_callback = nullptr;
				}

		template<DIST_GRAPH_PARAMS>
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>& DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::operator=(DistributedGraph<DIST_GRAPH_PARAMS_SPEC>&& graph) {
				// Calls base Graph move assignment
				static_cast<Graph<api::graph::DistributedNode<T>, api::graph::DistributedEdge<T>>&>(*this)
					= std::move(static_cast<Graph<api::graph::DistributedNode<T>, api::graph::DistributedEdge<T>>&>(graph));
				// Reassigns MPI communicators
				//
				// Notice that TypedMpi instances, location_manager and
				// sync_mode have been constructed in `graph` using
				// `graph.mpi_communicator`. Since we reassign
				// `this->mpi_communicator` to `graph.mpi_communicator`, we can
				// safely move those member fields from `graph` to `this`
				// without introducing inconsistencies.
				this->mpi_communicator = graph.mpi_communicator;
				this->id_mpi = std::move(graph.id_mpi);
				this->location_mpi = std::move(graph.location_mpi);

				this->location_manager = std::move(graph.location_manager);
				this->sync_mode = std::move(graph.sync_mode);
				// The obsolete `this->erase_node_callback` is
				// automatically deleted by the base Graph move assignment,
				// when the "erase_node_callback" list is cleared.
				this->erase_node_callback = graph.erase_node_callback;
				// Prevents erase_node_callback->disable() when `graph` is
				// deleted
				graph.erase_node_callback = nullptr;

				this->node_id = std::move(graph.node_id);
				this->edge_id = std::move(graph.edge_id);
				for(auto callback : set_local_callbacks)
					delete callback;
				for(auto callback : set_distant_callbacks)
					delete callback;
				this->set_local_callbacks = std::move(graph.set_local_callbacks);
				this->set_distant_callbacks = std::move(graph.set_distant_callbacks);

				return *this;
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setLocal(
					api::graph::DistributedNode<T>* node,
					typename api::graph::SetLocalNodeEvent<T>::Context context) {
				location_manager.setLocal(node);
				triggerSetLocalCallbacks({node, context});
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setDistant(
					api::graph::DistributedNode<T>* node,
					typename api::graph::SetDistantNodeEvent<T>::Context context) {
				location_manager.setDistant(node);
				triggerSetDistantCallbacks({node, context});
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(EdgeType* edge) {
				sync_mode.getSyncLinker().unlink(edge);
				this->erase(edge);
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::switchLayer(
					EdgeType* edge, api::graph::LayerId layer_id) {
				assert(edge->state() == api::graph::LOCAL);

				edge->getSourceNode()->unlinkOut(edge);
				edge->getTargetNode()->unlinkIn(edge);
				edge->setLayer(layer_id);
				edge->getSourceNode()->linkOut(edge);
				edge->getTargetNode()->linkIn(edge);
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(NodeType* node) {
				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing node %s...", FPMAS_C_STR(node->getId()));
				// The input node must be a temporary dynamically allocated object.
				// A representation of the node might already be contained in the
				// graph, if it were already built as a "distant" node.

				NodeType* local_node;
				const auto& nodes = this->getNodes();
				auto it = nodes.find(node->getId());
				if(it == nodes.end()) {
					FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Inserting new LOCAL node %s.", FPMAS_C_STR(node->getId()));
					// The node is not contained in the graph, we need to build a new
					// one.
					// But instead of completely building a new node, we can re-use the
					// temporary input node.
					this->insert(node);
					setLocal(node, api::graph::SetLocalNodeEvent<T>::IMPORT_NEW_LOCAL);
					node->setMutex(sync_mode.buildMutex(node));
					local_node = node;
				} else {
					FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Replacing existing DISTANT node %s.", FPMAS_C_STR(node->getId()));
					// A representation of the node was already contained in the graph.
					// We just need to update its state.

					// Set local representation as local
					local_node = it->second;
					synchro::DataUpdate<T>::update(
							local_node->data(), std::move(node->data())
							);
					local_node->setWeight(node->getWeight());
					setLocal(local_node, api::graph::SetLocalNodeEvent<T>::IMPORT_EXISTING_LOCAL);

					// Deletes unused temporary input node
					delete node;
				}
				for(auto& edge : local_node->getIncomingEdges())
					if(edge->state() == api::graph::DISTANT
							&& edge->getSourceNode()->state() == api::graph::LOCAL)
						edge->setState(api::graph::LOCAL);
				for(auto& edge : local_node->getOutgoingEdges())
					if(edge->state() == api::graph::DISTANT
							&& edge->getTargetNode()->state() == api::graph::LOCAL)
						edge->setState(api::graph::LOCAL);


				return local_node;
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importEdge(EdgeType* edge) {
				std::unique_ptr<api::graph::TemporaryNode<T>> temp_src
					= edge->getTempSourceNode();
				std::unique_ptr<api::graph::TemporaryNode<T>> temp_tgt
					= edge->getTempTargetNode();

				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing edge %s (from %s to %s)...",
						FPMAS_C_STR(edge->getId()),
						FPMAS_C_STR(temp_src->getId()),
						FPMAS_C_STR(temp_tgt->getId())
						);
				// The input edge must be a dynamically allocated object, with temporary
				// dynamically allocated nodes as source and target.
				// A representation of the imported edge might already be present in the
				// graph, for example if it has already been imported as a "distant"
				// edge with other nodes at other epochs.

				const auto& nodes = this->getNodes();
				const auto& edges = this->getEdges();
				auto edge_it = edges.find(edge->getId());
				if(edge_it == edges.end()) {
					// The edge does not belong to the graph : a new one must be built.

					DistributedId src_id = temp_src->getId();
					NodeType* src;
					DistributedId tgt_id =  temp_tgt->getId();
					NodeType* tgt;

					LocationState edgeLocationState = LocationState::LOCAL;

					auto src_it = nodes.find(src_id);
					if(src_it != nodes.end()) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing source %s", FPMAS_C_STR(src_id));
						// The source node is already contained in the graph
						src = src_it->second;
						if(src->state() == LocationState::DISTANT) {
							// At least src is DISTANT, so the imported edge is
							// necessarily DISTANT.
							edgeLocationState = LocationState::DISTANT;
						}
						// Deletes the temporary source node
						temp_src.reset();

						// Links the temporary edge with the src contained in the graph
						edge->setSourceNode(src);
						src->linkOut(edge);
					} else {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT source %s", FPMAS_C_STR(src_id));
						// The source node is not contained in the graph : it must be
						// built as a DISTANT node.
						edgeLocationState = LocationState::DISTANT;

						// Instead of building a new node, we re-use the temporary
						// source node.
						src = temp_src->build();
						temp_src.reset();

						edge->setSourceNode(src);
						src->linkOut(edge);
						this->insert(src);
						setDistant(src, api::graph::SetDistantNodeEvent<T>::IMPORT_NEW_DISTANT);
						src->setMutex(sync_mode.buildMutex(src));
						unsynchronized_nodes.insert(src);
					}
					auto tgt_it = nodes.find(tgt_id);
					if(tgt_it != nodes.end()) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing target %s", FPMAS_C_STR(tgt_id));
						// The target node is already contained in the graph
						tgt = tgt_it->second;
						if(tgt->state() == LocationState::DISTANT) {
							// At least src is DISTANT, so the imported edge is
							// necessarily DISTANT.
							edgeLocationState = LocationState::DISTANT;
						}
						// Deletes the temporary target node
						temp_tgt.reset();

						// Links the temporary edge with the tgt contained in the graph
						edge->setTargetNode(tgt);
						tgt->linkIn(edge);
					} else {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT target %s", FPMAS_C_STR(tgt_id));
						// The target node is not contained in the graph : it must be
						// built as a DISTANT node.
						edgeLocationState = LocationState::DISTANT;

						// Instead of building a new node, we re-use the temporary
						// target node.
						tgt = temp_tgt->build();
						temp_tgt.reset();

						edge->setTargetNode(tgt);
						tgt->linkIn(edge);
						this->insert(tgt);
						setDistant(tgt, api::graph::SetDistantNodeEvent<T>::IMPORT_NEW_DISTANT);
						tgt->setMutex(sync_mode.buildMutex(tgt));
						unsynchronized_nodes.insert(tgt);
					}
					// Finally, insert the temporary edge into the graph.
					edge->setState(edgeLocationState);
					this->insert(edge);
					return edge;
				} // if (graph.count(edge_id) > 0)

				// A representation of the edge is already present in the graph : it is
				// useless to insert it again. We just need to update its state.

				auto local_edge = edge_it->second;
				if(local_edge->getSourceNode()->state() == LocationState::LOCAL
						&& local_edge->getTargetNode()->state() == LocationState::LOCAL) {
					local_edge->setState(LocationState::LOCAL);
				}

				// Completely deletes temporary items, nothing is re-used
				// Temporary nodes are automatically deleted
				delete edge;

				return local_edge;
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::_buildNode(NodeType* node) {
				this->insert(node);
				setLocal(node, api::graph::SetLocalNodeEvent<T>::BUILD_LOCAL);
				location_manager.addManagedNode(node->getId(), mpi_communicator->getRank());
				node->setMutex(sync_mode.buildMutex(node));
				return node;
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(T&& data) {
				return _buildNode(new DistNodeImpl<T>(
							this->node_id++, std::move(data)
							));
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(const T& data) {
				return _buildNode(new DistNodeImpl<T>(
							this->node_id++, data
							));
			}
		template<DIST_GRAPH_PARAMS>
			api::graph::DistributedNode<T>* DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::insertDistant(
					api::graph::DistributedNode<T> * node) {
				FPMAS_LOGD(getMpiCommunicator().getRank(),
						"GRAPH", "Inserting temporary distant node: %s",
						FPMAS_C_STR(node->getId()));
				const auto& nodes = this->getNodes();
				auto node_it = nodes.find(node->getId());
				if(node_it == nodes.end()) {
					this->insert(node);
					setDistant(node, api::graph::SetDistantNodeEvent<T>::IMPORT_NEW_DISTANT);
					node->setMutex(sync_mode.buildMutex(node));
					unsynchronized_nodes.insert(node);
					return node;
				} else {
					auto id = node->getId();
					delete node;
					return node_it->second;
				}
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::removeNode(
					api::graph::DistributedNode<T> * node) {
				sync_mode.getSyncLinker().removeNode(node);
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::link(
					NodeType* const src, NodeType* const tgt, api::graph::LayerId layer) {
				// Builds the new edge
				auto edge = new DistEdgeImpl<T>(
						this->edge_id++, layer
						);
				edge->setSourceNode(src);
				src->linkOut(edge);
				edge->setTargetNode(tgt);
				tgt->linkIn(edge);

				edge->setState(
						src->state() == LocationState::LOCAL && tgt->state() == LocationState::LOCAL ?
						LocationState::LOCAL :
						LocationState::DISTANT
						);
				sync_mode.getSyncLinker().link(edge);

				// Inserts the edge in the Graph
				this->insert(edge);

				return edge;
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::distribute(api::graph::PartitionMap partition) {

				sync_mode.getSyncLinker().synchronize();

				_distribute(partition);

				sync_mode.getDataSync().synchronize();
			}


		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::_distribute(api::graph::PartitionMap partition) {
				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"Distributing graph...", "");
				std::string partition_str = "\n";
				for(auto item : partition) {
					std::string str = FPMAS_C_STR(item.first);
					str.append(" : " + std::to_string(item.second) + "\n");
					partition_str.append(str);
				}
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Partition : %s", partition_str.c_str());

				std::unordered_map<int, communication::DataPack> serial_nodes;
				std::unordered_map<int, communication::DataPack> serial_edges;

				std::vector<NodeType*> exported_nodes;
				{
					// Builds node and edges export maps
					std::unordered_map<int, std::vector<NodePtrWrapper<T>>> node_export_map;
					std::unordered_map<int, std::set<DistributedId>> edge_ids_to_export;
					std::unordered_map<int, std::vector<EdgePtrWrapper<T>>> edge_export_map;
					for(auto item : partition) {
						if(item.second != mpi_communicator->getRank()) {
							auto node = this->getNodes().find(item.first);
							if(node != this->getNodes().end()
									&& node->second->state() == api::graph::LOCAL) {
								FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH",
										"Exporting node %s to %i", FPMAS_C_STR(item.first), item.second);
								auto node_to_export = node->second;
								exported_nodes.push_back(node_to_export);
								node_export_map[item.second].emplace_back(node_to_export);
								for(auto edge :  node_to_export->getIncomingEdges()) {
									// If the target node is local on the
									// destination process, it already owns this
									// edge so no need to transmit it
									if(edge->getSourceNode()->location() != item.second) {
										// Insert or replace in the IDs set
										edge_ids_to_export[item.second].insert(edge->getId());
									}
								}
								for(auto edge :  node_to_export->getOutgoingEdges()) {
									// Same as above
									if(edge->getTargetNode()->location() != item.second) {
										// Insert or replace in the IDs set
										edge_ids_to_export[item.second].insert(edge->getId());
									}
								}
							}
						}
					}
					// Ensures that each edge is exported once to each process
					for(auto list : edge_ids_to_export) {
						for(auto id : list.second) {
							edge_export_map[list.first].emplace_back(this->getEdge(id));
						}
					}

					// Serializes nodes and edges
					for(auto& item : node_export_map)
						serial_nodes.emplace(
								item.first,
								io::datapack::ObjectPack(item.second).dump()
								);
					for(auto& item : edge_export_map)
						serial_edges.emplace(
								item.first,
								io::datapack::LightObjectPack(item.second).dump()
								);

					// Once serialized nodes and associated edges can
					// eventually be cleared. This might save memory for the
					// next import.

					for(auto node : exported_nodes) {
						setDistant(node, api::graph::SetDistantNodeEvent<T>::EXPORT_DISTANT);
					}

					// Clears buffers required for serialization
				}

				{
					// Node import
					std::unordered_map<int, std::vector<NodePtrWrapper<T>>> node_import;
					for(auto& item : mpi_communicator->allToAll(std::move(serial_nodes), MPI_CHAR))
						node_import.emplace(
								item.first, io::datapack::ObjectPack::parse(
									std::move(item.second)
									).get<std::vector<NodePtrWrapper<T>>>()
								);

					for(auto& import_node_list_from_proc : node_import) {
						for(auto& imported_node : import_node_list_from_proc.second) {
							this->importNode(imported_node);
						}
						import_node_list_from_proc.second.clear();
					}
					// node_import is deleted at the end of this scope
				}

				{
					// Edge import
					std::unordered_map<int, std::vector<EdgePtrWrapper<T>>> edge_import;

					for(auto& item : mpi_communicator->allToAll(std::move(serial_edges), MPI_CHAR))
						edge_import.emplace(
								item.first, io::datapack::LightObjectPack::parse(
									std::move(item.second)
									).get<std::vector<EdgePtrWrapper<T>>>()
								);

					for(auto& import_edge_list_from_proc : edge_import) {
						for(auto& imported_edge : import_edge_list_from_proc.second) {
							this->importEdge(imported_edge);
						}
						import_edge_list_from_proc.second.clear();
					}
					// edge_import is deleted at the end of this scope
				}
				

				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Clearing exported nodes...", "");
				for(auto node : exported_nodes) {
					clearNode(node);
				}

				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Exported nodes cleared.", "");


				location_manager.updateLocations();

				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"End of distribution.", "");
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::synchronize() {
				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"Synchronizing graph...", "");

				sync_mode.getSyncLinker().synchronize();

				clearDistantNodes();

				sync_mode.getDataSync().synchronize();

				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"End of graph synchronization.", "");
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::synchronize(std::unordered_set<NodeType*> nodes, bool synchronize_links) {
				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"Partially synchronizing graph...", "");

				if(synchronize_links) {
					sync_mode.getSyncLinker().synchronize();
					clearDistantNodes();
				}

				sync_mode.getDataSync().synchronize(nodes);

				for(auto node : nodes)
					unsynchronized_nodes.erase(node);

				FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
						"End of partial graph synchronization.", "");
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::clearDistantNodes() {
				NodeMap distant_nodes = location_manager.getDistantNodes();
				for(auto node : distant_nodes)
					clearNode(node.second);
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
			::clearNode(NodeType* node) {
				DistributedId id = node->getId();
				FPMAS_LOGD(
						mpi_communicator->getRank(),
						"DIST_GRAPH", "Clearing node %s...",
						FPMAS_C_STR(id)
						);

				bool eraseNode = true;
				std::set<EdgeType*> obsoleteEdges;
				for(auto edge : node->getIncomingEdges()) {
					if(edge->getSourceNode()->state()==LocationState::LOCAL) {
						eraseNode = false;
					} else {
						obsoleteEdges.insert(edge);
					}
				}
				for(auto edge : node->getOutgoingEdges()) {
					if(edge->getTargetNode()->state()==LocationState::LOCAL) {
						eraseNode = false;
					} else {
						obsoleteEdges.insert(edge);
					}
				}
				if(eraseNode) {
					FPMAS_LOGD(
							mpi_communicator->getRank(),
							"DIST_GRAPH", "Erasing obsolete node %s.",
							FPMAS_C_STR(node->getId())
							);
					this->erase(node);
				} else {
					for(auto edge : obsoleteEdges) {
						FPMAS_LOGD(
								mpi_communicator->getRank(),
								"DIST_GRAPH", "Erasing obsolete edge %s (%p) (from %s to %s)",
								FPMAS_C_STR(node->getId()), edge,
								FPMAS_C_STR(edge->getSourceNode()->getId()),
								FPMAS_C_STR(edge->getTargetNode()->getId())
								);
						this->erase(edge);
					}
				}
				FPMAS_LOGD(
						mpi_communicator->getRank(),
						"DIST_GRAPH", "Node %s cleared.",
						FPMAS_C_STR(id)
						);
			}

		template<DIST_GRAPH_PARAMS>
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::~DistributedGraph() {
				//TODO: Refactor callback system
				if(erase_node_callback != nullptr)
					// This DistributedGraph has not been moved
					erase_node_callback->disable();
				for(auto callback : set_local_callbacks)
					delete callback;
				for(auto callback : set_distant_callbacks)
					delete callback;
			}

	}

	/**
	 * Default api::graph::DistributedGraph implementation.
	 *
	 * @see detail::DistributedGraph
	 */
	template<typename T, template<typename> class SyncMode>
		using DistributedGraph = detail::DistributedGraph<
		T, SyncMode, graph::DistributedNode, graph::DistributedEdge,
		communication::TypedMpi, graph::LocationManager>;

}}

namespace nlohmann {
	/**
	 *
	 * Defines Json serialization rules for generic
	 * fpmas::api::graph::DistributedGraph instances.
	 *
	 * This notably enables the usage of fpmas::io::Breakpoint with
	 * fpmas::api::graph::DistributedGraph instances.
	 *
	 */
	template<typename T>
		class adl_serializer<fpmas::api::graph::DistributedGraph<T>> {
			private:
				typedef fpmas::api::graph::DistributedId DistributedId;

				class NullTemporaryNode : public fpmas::api::graph::TemporaryNode<T> {
					private:
						DistributedId id;
					public:
						NullTemporaryNode(DistributedId id) : id(id) {
						}

						DistributedId getId() const override {
							return id;
						}

						int getLocation() const override {
							assert(false);
							return -1;
						}

						fpmas::api::graph::DistributedNode<T>* build() override {
							assert(false);
							return nullptr;
						}
				};

			public:

				/**
				 * \anchor nlohmann_adl_serializer_DistributedGraph_to_json
				 *
				 * Serializes `graph` to the json `j`.
				 *
				 * @param j json output
				 * @param graph graph to serialize
				 */
				static void to_json(json& j, const fpmas::api::graph::DistributedGraph<T>& graph) {
					// Specifies the rank on which the local distributed graph
					// was built
					j["rank"] = graph.getMpiCommunicator().getRank();
					for(auto node : graph.getNodes())
						j["graph"]["nodes"].push_back({
								NodePtrWrapper<T>(node.second),
								node.second->location()
								});
					for(auto edge : graph.getEdges()) {
						j["graph"]["edges"].push_back({
								{"id", edge.first},
								{"layer", edge.second->getLayer()},
								{"weight", edge.second->getWeight()},
								{"src", edge.second->getSourceNode()->getId()},
								{"tgt", edge.second->getTargetNode()->getId()}
								});
					}
					j["edge_id"] = graph.currentEdgeId();
					j["node_id"] = graph.currentNodeId();
					j["loc_manager"] = graph.getLocationManager();
					nlohmann::json::json_serializer<fpmas::api::graph::LocationManager<T>, void>
						::to_json(j["loc_manager"], graph.getLocationManager());
				}

				/**
				 * \anchor nlohmann_adl_serializer_DistributedGraph_from_json
				 *
				 * Unserializes the json `j` into the specified `graph`.
				 *
				 * Nodes and edges read from the Json file are imported into the
				 * `graph`, without altering the previous state of the input
				 * `graph`. So, depending on the desired behavior, calling
				 * `graph.clear()` might be required before loading data into it.
				 *
				 * @param j json input
				 * @param graph output graph
				 */
				static void from_json(const json& j, fpmas::api::graph::DistributedGraph<T>& graph) {
					auto j_graph = j["graph"];
					for(auto j_node : j_graph["nodes"]) {
						auto location = j_node[1].get<int>();
						fpmas::api::graph::DistributedNode<T>* node
							= j_node[0].get<NodePtrWrapper<T>>();
						if(location == graph.getMpiCommunicator().getRank())
							node = graph.importNode(node);
						else
							node = graph.insertDistant(node);

						node->setLocation(location);
					}
					for(auto j_edge : j_graph["edges"]) {
						auto edge = new fpmas::graph::DistributedEdge<T>(
								j_edge["id"].get<fpmas::graph::DistributedId>(),
								j_edge["layer"].get<fpmas::graph::LayerId>()
								);
						edge->setWeight(j_edge["weight"].get<float>());
						// Temp source and target are never used (expect for
						// their ids) since source and target are necessarily
						// contained in the graph (see previous step)
						auto src_id = j_edge["src"].get<DistributedId>();
						edge->setTempSourceNode(
								std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
									new NullTemporaryNode(src_id)
									)
								);
						auto tgt_id = j_edge["tgt"].get<DistributedId>();
						edge->setTempTargetNode(
								std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>(
									new NullTemporaryNode(tgt_id)
									)
								);
						assert(graph.getNodes().count(src_id) > 0);
						assert(graph.getNodes().count(tgt_id) > 0);
						graph.importEdge(edge);
					}
					graph.setCurrentNodeId(j.at("node_id").get<DistributedId>());
					graph.setCurrentEdgeId(j.at("edge_id").get<DistributedId>());

					nlohmann::json::json_serializer<fpmas::api::graph::LocationManager<T>, void>
						::from_json(j["loc_manager"], graph.getLocationManager());
				}
		};
}
#endif
