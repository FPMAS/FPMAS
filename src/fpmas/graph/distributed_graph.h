#ifndef FPMAS_DISTRIBUTED_GRAPH_H
#define FPMAS_DISTRIBUTED_GRAPH_H

/** \file src/fpmas/graph/distributed_graph.h
 * DistributedGraph implementation.
 */

#include <set>

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/graph/location_manager.h"

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
				using typename api::graph::DistributedGraph<T>::NodeCallback;

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
							if(enabled)
								graph.location_manager.remove(node);
						}
				};

				api::communication::MpiCommunicator& mpi_communicator;
				TypedMpi<DistributedId> id_mpi {mpi_communicator};
				TypedMpi<std::pair<DistributedId, int>> location_mpi {mpi_communicator};
				TypedMpi<NodePtrWrapper<T>> node_mpi {mpi_communicator};
				TypedMpi<EdgePtrWrapper<T>> edge_mpi {mpi_communicator};

				LocationManagerImpl<T> location_manager;
				SyncMode<T> sync_mode;

				std::vector<NodeCallback*> set_local_callbacks;
				std::vector<NodeCallback*> set_distant_callbacks;

				EraseNodeCallback* erase_node_callback;

				NodeType* _buildNode(NodeType*);

				void setLocal(api::graph::DistributedNode<T>* node);
				void setDistant(api::graph::DistributedNode<T>* node);

				void triggerSetLocalCallbacks(api::graph::DistributedNode<T>* node) {
					for(auto callback : set_local_callbacks)
						callback->call(node);
				}

				void triggerSetDistantCallbacks(api::graph::DistributedNode<T>* node) {
					for(auto callback : set_distant_callbacks)
						callback->call(node);
				}

				void clearDistantNodes();
				void clearNode(NodeType*);
				void _distribute(api::graph::PartitionMap);

				DistributedId node_id;
				DistributedId edge_id;

			public:
				/**
				 * DistributedGraph constructor.
				 *
				 * @param comm MPI communicator
				 */
				DistributedGraph(api::communication::MpiCommunicator& comm) :
					mpi_communicator(comm), location_manager(mpi_communicator, id_mpi, location_mpi),
					sync_mode(*this, mpi_communicator),
					erase_node_callback(new EraseNodeCallback(*this)),
					node_id(mpi_communicator.getRank(), 0),
					edge_id(mpi_communicator.getRank(), 0)
			{
				this->addCallOnEraseNode(erase_node_callback);
			}

				/**
				 * Returns the internal MpiCommunicator.
				 *
				 * @return reference to the internal MpiCommunicator
				 */
				api::communication::MpiCommunicator& getMpiCommunicator() override {
					return mpi_communicator;
				};

				/**
				 * \copydoc getMpiCommunicator()
				 */
				const api::communication::MpiCommunicator& getMpiCommunicator() const override {
					return mpi_communicator;
				};

				/**
				 * Returns the internal TypedMpi used to transmit NodePtrWrapper
				 * instances.
				 *
				 * @return NodePtrWrapper TypedMpi
				 */
				const TypedMpi<NodePtrWrapper<T>>& getNodeMpi() const {return node_mpi;}
				/**
				 * Returns the internal TypedMpi used to transmit EdgePtrWrapper
				 * instances.
				 *
				 * @return EdgePtrWrapper TypedMpi
				 */
				const TypedMpi<EdgePtrWrapper<T>>& getEdgeMpi() const {return edge_mpi;}

				const DistributedId& currentNodeId() const override {return node_id;}
				const DistributedId& currentEdgeId() const override {return edge_id;}

				NodeType* importNode(NodeType* node) override;
				EdgeType* importEdge(EdgeType* edge) override;


				/**
				 * Returns the internal SyncMode instance.
				 *
				 * @return sync mode
				 */
				const SyncMode<T>& getSyncMode() const {return sync_mode;}

				const LocationManagerImpl<T>&
					getLocationManager() const override {return location_manager;}

				void balance(api::graph::LoadBalancing<T>& load_balancing) override {
					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Balancing graph (%lu nodes, %lu edges)",
							this->getNodes().size(), this->getEdges().size());

					sync_mode.getSyncLinker().synchronize();
					this->_distribute(load_balancing.balance(this->location_manager.getLocalNodes()));
					sync_mode.getDataSync().synchronize();

					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Graph balanced : %lu nodes, %lu edges",
							this->getNodes().size(), this->getEdges().size());
				};

				void balance(
						api::graph::FixedVerticesLoadBalancing<T>& load_balancing,
						api::graph::PartitionMap fixed_vertices
						) override {
					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Balancing graph (%lu nodes, %lu edges)",
							this->getNodes().size(), this->getEdges().size());

					sync_mode.getSyncLinker().synchronize();
					this->_distribute(load_balancing.balance(this->location_manager.getLocalNodes(), fixed_vertices));
					sync_mode.getDataSync().synchronize();

					FPMAS_LOGI(
							getMpiCommunicator().getRank(), "LB",
							"Graph balanced : %lu nodes, %lu edges",
							this->getNodes().size(), this->getEdges().size());
				};

				void distribute(api::graph::PartitionMap partition) override;

				void synchronize() override;


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

				void addCallOnSetLocal(NodeCallback* callback) override {
					set_local_callbacks.push_back(callback);
				};

				void addCallOnSetDistant(NodeCallback* callback) override {
					set_distant_callbacks.push_back(callback);
				};

				~DistributedGraph();
		};

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setLocal(api::graph::DistributedNode<T>* node) {
				location_manager.setLocal(node);
				triggerSetLocalCallbacks(node);
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setDistant(api::graph::DistributedNode<T>* node) {
				location_manager.setDistant(node);
				triggerSetDistantCallbacks(node);
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(EdgeType* edge) {
				auto src = edge->getSourceNode();
				auto tgt = edge->getTargetNode();
				src->mutex()->lockShared();
				tgt->mutex()->lockShared();

				sync_mode.getSyncLinker().unlink(edge);

				this->erase(edge);

				src->mutex()->unlockShared();
				tgt->mutex()->unlockShared();
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(NodeType* node) {
				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing node %s...", FPMAS_C_STR(node->getId()));
				// The input node must be a temporary dynamically allocated object.
				// A representation of the node might already be contained in the
				// graph, if it were already built as a "distant" node.

				if(this->getNodes().count(node->getId())==0) {
					FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Inserting new LOCAL node %s.", FPMAS_C_STR(node->getId()));
					// The node is not contained in the graph, we need to build a new
					// one.
					// But instead of completely building a new node, we can re-use the
					// temporary input node.
					this->insert(node);
					setLocal(node);
					node->setMutex(sync_mode.buildMutex(node));
					return node;
				}
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Replacing existing DISTANT node %s.", FPMAS_C_STR(node->getId()));
				// A representation of the node was already contained in the graph.
				// We just need to update its state.

				// Set local representation as local
				auto local_node = this->getNode(node->getId());
				local_node->data() = std::move(node->data());
				local_node->setWeight(node->getWeight());
				setLocal(local_node);

				// Deletes unused temporary input node
				delete node;

				return local_node;
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importEdge(EdgeType* edge) {
				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing edge %s (from %s to %s)...",
						FPMAS_C_STR(edge->getId()),
						FPMAS_C_STR(edge->getSourceNode()->getId()),
						FPMAS_C_STR(edge->getTargetNode()->getId())
						);
				// The input edge must be a dynamically allocated object, with temporary
				// dynamically allocated nodes as source and target.
				// A representation of the imported edge might already be present in the
				// graph, for example if it has already been imported as a "distant"
				// edge with other nodes at other epochs.

				if(this->getEdges().count(edge->getId())==0) {
					// The edge does not belong to the graph : a new one must be built.

					DistributedId srcId = edge->getSourceNode()->getId();
					NodeType* src;
					DistributedId tgtId =  edge->getTargetNode()->getId();
					NodeType* tgt;

					LocationState edgeLocationState = LocationState::LOCAL;

					if(this->getNodes().count(srcId) > 0) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing source %s", FPMAS_C_STR(srcId));
						// The source node is already contained in the graph
						src = this->getNode(srcId);
						if(src->state() == LocationState::DISTANT) {
							// At least src is DISTANT, so the imported edge is
							// necessarily DISTANT.
							edgeLocationState = LocationState::DISTANT;
						}
						// Deletes the temporary source node
						delete edge->getSourceNode();

						// Links the temporary edge with the src contained in the graph
						edge->setSourceNode(src);
						src->linkOut(edge);
					} else {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT source %s", FPMAS_C_STR(srcId));
						// The source node is not contained in the graph : it must be
						// built as a DISTANT node.
						edgeLocationState = LocationState::DISTANT;

						// Instead of building a new node, we re-use the temporary
						// source node.
						src = edge->getSourceNode();
						this->insert(src);
						setDistant(src);
						src->setMutex(sync_mode.buildMutex(src));
					}
					if(this->getNodes().count(tgtId) > 0) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing target %s", FPMAS_C_STR(tgtId));
						// The target node is already contained in the graph
						tgt = this->getNode(tgtId);
						if(tgt->state() == LocationState::DISTANT) {
							// At least src is DISTANT, so the imported edge is
							// necessarily DISTANT.
							edgeLocationState = LocationState::DISTANT;
						}
						// Deletes the temporary target node
						delete edge->getTargetNode();

						// Links the temporary edge with the tgt contained in the graph
						edge->setTargetNode(tgt);
						tgt->linkIn(edge);
					} else {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT target %s", FPMAS_C_STR(tgtId));
						// The target node is not contained in the graph : it must be
						// built as a DISTANT node.
						edgeLocationState = LocationState::DISTANT;

						// Instead of building a new node, we re-use the temporary
						// target node.
						tgt = edge->getTargetNode();
						this->insert(tgt);
						setDistant(tgt);
						tgt->setMutex(sync_mode.buildMutex(tgt));
					}
					// Finally, insert the temporary edge into the graph.
					edge->setState(edgeLocationState);
					this->insert(edge);
					return edge;
				} // if (graph.count(edge_id) > 0)

				// A representation of the edge is already present in the graph : it is
				// useless to insert it again. We just need to update its state.

				auto local_edge = this->getEdge(edge->getId());
				if(local_edge->getSourceNode()->state() == LocationState::LOCAL
						&& local_edge->getTargetNode()->state() == LocationState::LOCAL) {
					local_edge->setState(LocationState::LOCAL);
				}

				// Completely deletes temporary items, nothing is re-used
				delete edge->getSourceNode();
				delete edge->getTargetNode();
				delete edge;

				return local_edge;
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::_buildNode(NodeType* node) {
				this->insert(node);
				setLocal(node);
				location_manager.addManagedNode(node, mpi_communicator.getRank());
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
				if(this->getNodes().count(node->getId()) == 0) {
					this->insert(node);
					setDistant(node);
					node->setMutex(sync_mode.buildMutex(node));
					return node;
				} else {
					auto id = node->getId();
					delete node;
					return this->getNode(id);
				}
			}

		template<DIST_GRAPH_PARAMS>
			void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::removeNode(
					api::graph::DistributedNode<T> * node) {
				node->mutex()->lockShared();

				sync_mode.getSyncLinker().removeNode(node);

				node->mutex()->unlockShared();
			}

		template<DIST_GRAPH_PARAMS>
			typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::EdgeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::link(
					NodeType* const src, NodeType* const tgt, api::graph::LayerId layer) {
				// Locks source and target
				src->mutex()->lockShared();
				tgt->mutex()->lockShared();

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

				// Unlocks source and target
				src->mutex()->unlockShared();
				tgt->mutex()->unlockShared();

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


				// Builds node and edges export maps
				std::vector<NodeType*> exported_nodes;
				std::unordered_map<int, std::vector<NodePtrWrapper<T>>> node_export_map;
				std::unordered_map<int, std::set<DistributedId>> edge_ids_to_export;
				std::unordered_map<int, std::vector<EdgePtrWrapper<T>>> edge_export_map;
				for(auto item : partition) {
					if(this->getNodes().count(item.first) > 0) {
						if(item.second != mpi_communicator.getRank()) {
							FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH",
									"Exporting node %s to %i", FPMAS_C_STR(item.first), item.second);
							auto node_to_export = this->getNode(item.first);
							exported_nodes.push_back(node_to_export);
							node_export_map[item.second].emplace_back(node_to_export);
							for(auto edge :  node_to_export->getIncomingEdges()) {
								// Insert or replace in the IDs set
								edge_ids_to_export[item.second].insert(edge->getId());
							}
							for(auto edge :  node_to_export->getOutgoingEdges()) {
								// Insert or replace in the IDs set
								edge_ids_to_export[item.second].insert(edge->getId());
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

				// Serialize and export / import nodes
				auto node_import = node_mpi.migrate(node_export_map);

				// Serialize and export / import edges
				auto edge_import = edge_mpi.migrate(edge_export_map);

				for(auto& import_node_list_from_proc : node_import) {
					for(auto& imported_node : import_node_list_from_proc.second) {
						this->importNode(imported_node);
					}
				}
				for(auto& import_edge_list_from_proc : edge_import) {
					for(auto& imported_edge : import_edge_list_from_proc.second) {
						this->importEdge(imported_edge);
					}
				}

				for(auto node : exported_nodes) {
					setDistant(node);
				}

				location_manager.updateLocations();

				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Clearing exported nodes...", "");
				for(auto node : exported_nodes) {
					clearNode(node);
				}
				FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Exported nodes cleared.", "");

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
						mpi_communicator.getRank(),
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
							mpi_communicator.getRank(),
							"DIST_GRAPH", "Erasing obsolete node %s.",
							FPMAS_C_STR(node->getId())
							);
					this->erase(node);
				} else {
					for(auto edge : obsoleteEdges) {
						FPMAS_LOGD(
								mpi_communicator.getRank(),
								"DIST_GRAPH", "Erasing obsolete edge %s (%p) (from %s to %s)",
								FPMAS_C_STR(node->getId()), edge,
								FPMAS_C_STR(edge->getSourceNode()->getId()),
								FPMAS_C_STR(edge->getTargetNode()->getId())
								);
						this->erase(edge);
					}
				}
				FPMAS_LOGD(
						mpi_communicator.getRank(),
						"DIST_GRAPH", "Node %s cleared.",
						FPMAS_C_STR(id)
						);
			}

		template<DIST_GRAPH_PARAMS>
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::~DistributedGraph() {
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

}
}
#endif
