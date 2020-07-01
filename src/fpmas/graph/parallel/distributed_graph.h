#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "fpmas/api/graph/parallel/distributed_graph.h"
#include "fpmas/communication/communication.h"
#include "distributed_arc.h"
#include "distributed_node.h"
#include "location_manager.h"

#include "fpmas/graph/base/graph.h"

#define DIST_GRAPH_PARAMS\
	typename T,\
	typename SyncMode,\
	template<typename> class DistNodeImpl,\
	template<typename> class DistArcImpl,\
	typename MpiSetUp,\
	template<typename> class LocationManagerImpl

#define DIST_GRAPH_PARAMS_SPEC\
	T,\
	SyncMode,\
	DistNodeImpl,\
	DistArcImpl,\
	MpiSetUp,\
	LocationManagerImpl

namespace fpmas::graph::parallel {
	
	using fpmas::api::graph::parallel::LocationState;
	
	typedef api::communication::MpiSetUp<communication::MpiCommunicator, communication::TypedMpi> DefaultMpiSetUp;

	template<DIST_GRAPH_PARAMS>
	class DistributedGraph : 
		public base::Graph<
			api::graph::parallel::DistributedNode<T>,
			api::graph::parallel::DistributedArc<T>>,
		public api::graph::parallel::DistributedGraph<T>
		 {
			public:
			typedef DistNodeImpl<T> DistNodeType;
			typedef DistArcImpl<T> DistArcType;
			typedef typename SyncMode::template SyncModeRuntimeType<T> SyncModeRuntimeType;

			static_assert(
					std::is_base_of<api::graph::parallel::DistributedNode<T>, DistNodeType>::value,
					"DistNodeImpl must implement api::graph::parallel::DistributedNode"
					);
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedArc<T>, DistArcType>::value,
					"DistArcImpl must implement api::graph::parallel::DistributedArc"
					);
			typedef base::Graph<api::graph::parallel::DistributedNode<T>, api::graph::parallel::DistributedArc<T>>
			Graph;
			typedef api::graph::parallel::DistributedGraph<T> DistGraphBase;

			public:
			typedef api::graph::parallel::DistributedNode<T> NodeType;
			typedef api::graph::parallel::DistributedArc<T> ArcType;
			using typename DistGraphBase::LayerIdType;
			using typename DistGraphBase::NodeMap;
			using typename DistGraphBase::PartitionMap;
			typedef typename MpiSetUp::communicator communicator;
			template<typename Data>
			using mpi = typename MpiSetUp::template mpi<Data>;
			using NodeCallback = typename DistGraphBase::NodeCallback;

			private:
			communicator mpi_communicator;
			mpi<DistributedId> id_mpi {mpi_communicator};
			mpi<std::pair<DistributedId, int>> location_mpi {mpi_communicator};
			mpi<NodePtrWrapper<T>> node_mpi {mpi_communicator};
			mpi<ArcPtrWrapper<T>> arc_mpi {mpi_communicator};

			LocationManagerImpl<T> location_manager;
			SyncModeRuntimeType sync_mode_runtime;

			std::vector<NodeCallback*> set_local_callbacks;
			std::vector<NodeCallback*> set_distant_callbacks;

			NodeType* buildNode(NodeType*);

			void setLocal(api::graph::parallel::DistributedNode<T>* node);
			void setDistant(api::graph::parallel::DistributedNode<T>* node);

			void triggerSetLocalCallbacks(api::graph::parallel::DistributedNode<T>* node) {
				for(auto callback : set_local_callbacks)
					callback->call(node);
			}

			void triggerSetDistantCallbacks(api::graph::parallel::DistributedNode<T>* node) {
				for(auto callback : set_distant_callbacks)
					callback->call(node);
			}


			public:
			DistributedGraph() :
				location_manager(mpi_communicator, id_mpi, location_mpi),
				sync_mode_runtime(*this, mpi_communicator) {
				// Initialization in the body of this (derived) class of the
				// (base) fields nodeId and arcId, to ensure that
				// mpi_communicator is initialized (as a field of this derived
				// class)
				this->node_id = DistributedId(mpi_communicator.getRank(), 0);
				this->arc_id = DistributedId(mpi_communicator.getRank(), 0);
			}

			const communicator& getMpiCommunicator() const override {
				return mpi_communicator;
			};
			communicator& getMpiCommunicator() {
				return mpi_communicator;
			};

			const mpi<NodePtrWrapper<T>>& getNodeMpi() const {return node_mpi;}
			const mpi<ArcPtrWrapper<T>>& getArcMpi() const {return arc_mpi;}

			void clearArc(ArcType*) override;
			void clearNode(NodeType*) override;

			NodeType* importNode(NodeType* node) override;
			ArcType* importArc(ArcType* arc) override;

			const SyncModeRuntimeType& getSyncModeRuntime() const {return sync_mode_runtime;}

			const LocationManagerImpl<T>&
				getLocationManager() const override {return location_manager;}

			void removeNode(NodeType*) override {};
			void unlink(ArcType*) override;

			void balance(api::load_balancing::LoadBalancing<T>& load_balancing) override {
				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Balancing graph (%lu nodes, %lu arcs)",
						this->getNodes().size(), this->getArcs().size());

				typename api::load_balancing::LoadBalancing<T>::ConstNodeMap node_map;
				for(auto node : this->location_manager.getLocalNodes()) {
					node_map.insert(node);
				}
				this->distribute(load_balancing.balance(node_map));

				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Graph balanced : %lu nodes, %lu arcs",
						this->getNodes().size(), this->getArcs().size());
			};

			void balance(api::load_balancing::FixedVerticesLoadBalancing<T>& load_balancing, PartitionMap fixed_nodes) override {
				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Balancing graph (%lu nodes, %lu arcs)",
						this->getNodes().size(), this->getArcs().size());

				typename api::load_balancing::LoadBalancing<T>::ConstNodeMap node_map;
				for(auto node : this->getNodes()) {
					node_map.insert(node);
				}
				this->distribute(load_balancing.balance(node_map, fixed_nodes));

				FPMAS_LOGI(
						getMpiCommunicator().getRank(), "LB",
						"Graph balanced : %lu nodes, %lu arcs",
						this->getNodes().size(), this->getArcs().size());
			};

			void distribute(PartitionMap partition) override;

			void synchronize() override;


			//NodeType* buildNode(const T&) override;
			NodeType* buildNode(T&& = std::move(T())) override;

			ArcType* link(NodeType* const src, NodeType* const tgt, LayerIdType layer) override;

			void addCallOnSetLocal(NodeCallback* callback) override {
				set_local_callbacks.push_back(callback);
			};

			void addCallOnSetDistant(NodeCallback* callback) override {
				set_distant_callbacks.push_back(callback);
			};

			~DistributedGraph();
		};

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setLocal(api::graph::parallel::DistributedNode<T>* node) {
			location_manager.setLocal(node);
			triggerSetLocalCallbacks(node);
		}

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::setDistant(api::graph::parallel::DistributedNode<T>* node) {
			location_manager.setDistant(node);
			triggerSetDistantCallbacks(node);
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(ArcType* arc) {
		auto src = arc->getSourceNode();
		auto tgt = arc->getTargetNode();
		src->mutex().lock();
		tgt->mutex().lock();

		sync_mode_runtime.getSyncLinker().unlink(static_cast<DistArcType*>(arc));
		// TODO : src and tgt might be cleared

		src->mutex().unlock();
		tgt->mutex().unlock();

		// TODO : clear usage here is risky, and does not support
		// multi-threading. It can't be called before src->unlock and
		// tgt->unlock, because if one of the two nodes is "cleared" in
		// clear(arc) src->mutex() will seg fault. 
		//
		// "clear" functions should globally be improved : what about a custom
		// GarbageCollector for the graph?
		//this->clearArc(arc);
		this->erase(arc);
	}

	template<DIST_GRAPH_PARAMS>
	typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
	DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(NodeType* node) {
		FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing node %s...", ID_C_STR(node->getId()));
		// The input node must be a temporary dynamically allocated object.
		// A representation of the node might already be contained in the
		// graph, if it were already built as a "distant" node.

		if(this->getNodes().count(node->getId())==0) {
			FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Inserting new LOCAL node %s.", ID_C_STR(node->getId()));
			// The node is not contained in the graph, we need to build a new
			// one.
			// But instead of completely building a new node, we can re-use the
			// temporary input node.
			this->insert(node);
			setLocal(node);
			node->setMutex(sync_mode_runtime.buildMutex(node));
			return node;
		}
		FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Replacing existing DISTANT node %s.", ID_C_STR(node->getId()));
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
	typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::ArcType*
	DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importArc(ArcType* arc) {
		FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Importing arc %s (from %s to %s)...",
				ID_C_STR(arc->getId()),
				ID_C_STR(arc->getSourceNode()->getId()),
				ID_C_STR(arc->getTargetNode()->getId())
				);
		// The input arc must be a dynamically allocated object, with temporary
		// dynamically allocated nodes as source and target.
		// A representation of the imported arc might already be present in the
		// graph, for example if it has already been imported as a "distant"
		// arc with other nodes at other epochs.

		if(this->getArcs().count(arc->getId())==0) {
			// The arc does not belong to the graph : a new one must be built.

			DistributedId srcId = arc->getSourceNode()->getId();
			NodeType* src;
			DistributedId tgtId =  arc->getTargetNode()->getId();
			NodeType* tgt;

			LocationState arcLocationState = LocationState::LOCAL;

			if(this->getNodes().count(srcId) > 0) {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing source %s", ID_C_STR(srcId));
				// The source node is already contained in the graph
				src = this->getNode(srcId);
				if(src->state() == LocationState::DISTANT) {
					// At least src is DISTANT, so the imported arc is
					// necessarily DISTANT.
					arcLocationState = LocationState::DISTANT;
				}
				// Deletes the temporary source node
				delete arc->getSourceNode();

				// Links the temporary arc with the src contained in the graph
				arc->setSourceNode(src);
				src->linkOut(arc);
			} else {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT source %s", ID_C_STR(srcId));
				// The source node is not contained in the graph : it must be
				// built as a DISTANT node.
				arcLocationState = LocationState::DISTANT;

				// Instead of building a new node, we re-use the temporary
				// source node.
				src = arc->getSourceNode();
				this->insert(src);
				setDistant(src);
				src->setMutex(sync_mode_runtime.buildMutex(src));
			}
			if(this->getNodes().count(tgtId) > 0) {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Linking existing target %s", ID_C_STR(tgtId));
				// The target node is already contained in the graph
				tgt = this->getNode(tgtId);
				if(tgt->state() == LocationState::DISTANT) {
					// At least src is DISTANT, so the imported arc is
					// necessarily DISTANT.
					arcLocationState = LocationState::DISTANT;
				}
				// Deletes the temporary target node
				delete arc->getTargetNode();

				// Links the temporary arc with the tgt contained in the graph
				arc->setTargetNode(tgt);
				tgt->linkIn(arc);
			} else {
				FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Creating DISTANT target %s", ID_C_STR(tgtId));
				// The target node is not contained in the graph : it must be
				// built as a DISTANT node.
				arcLocationState = LocationState::DISTANT;

				// Instead of building a new node, we re-use the temporary
				// target node.
				tgt = arc->getTargetNode();
				this->insert(tgt);
				setDistant(tgt);
				tgt->setMutex(sync_mode_runtime.buildMutex(tgt));
			}
			// Finally, insert the temporary arc into the graph.
			arc->setState(arcLocationState);
			this->insert(arc);
			return arc;
		} // if (graph.count(arc_id) > 0)

		// A representation of the arc is already present in the graph : it is
		// useless to insert it again. We just need to update its state.

		auto local_arc = this->getArc(arc->getId());
		if(local_arc->getSourceNode()->state() == LocationState::LOCAL
				&& local_arc->getTargetNode()->state() == LocationState::LOCAL) {
			local_arc->setState(LocationState::LOCAL);
		}

		// Completely deletes temporary items, nothing is re-used
		delete arc->getSourceNode();
		delete arc->getTargetNode();
		delete arc;

		return local_arc;
	}

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(NodeType* node) {
				this->insert(node);
				setLocal(node);
				location_manager.addManagedNode(node, mpi_communicator.getRank());
				node->setMutex(sync_mode_runtime.buildMutex(node));
				//sync_mode_runtime.setUp(node->getId(), dynamic_cast<typename SyncMode::template MutexType<T>&>(node->mutex()));
				return node;
			}

	/*
	 *template<DIST_GRAPH_PARAMS>
	 *    typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
	 *        DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(const T& data) {
	 *            return buildNode(new DistNodeType(
	 *                    this->node_id++, data
	 *                    ));
	 *        }
	 */

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::buildNode(T&& data) {
				return buildNode(new DistNodeType(
						this->node_id++, std::move(data)
						));
			}

	template<DIST_GRAPH_PARAMS>
		typename DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::ArcType*
			DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::link(NodeType* const src, NodeType* const tgt, LayerIdType layer) {
				// Locks source and target
				src->mutex().lock();
				tgt->mutex().lock();

				// Builds the new arc
				auto arc = new DistArcType(
						this->arc_id++, layer
						);
				arc->setSourceNode(src);
				src->linkOut(arc);
				arc->setTargetNode(tgt);
				tgt->linkIn(arc);

				arc->setState(
						src->state() == LocationState::LOCAL && tgt->state() == LocationState::LOCAL ?
						LocationState::LOCAL :
						LocationState::DISTANT
						);
				sync_mode_runtime.getSyncLinker().link(arc);

				// If src and tgt is DISTANT, transmit the request to the
				// SyncLinker, that will handle the request according to its
				// synchronisation policy
				/*
				 *if(src->state() == LocationState::DISTANT || tgt->state() == LocationState::DISTANT) {
				 *    sync_mode_runtime.getSyncLinker().link(arc);
				 *    arc->setState(LocationState::DISTANT);
				 *} else {
				 *    arc->setState(LocationState::LOCAL);
				 *}
				 */

				// Inserts the arc in the Graph
				this->insert(arc);

				// Unlocks source and target
				src->mutex().unlock();
				tgt->mutex().unlock();

				return arc;
			}


	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::distribute(PartitionMap partition) {
			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"Distributing graph...");
			std::string partition_str = "\n";
			for(auto item : partition) {
				std::string str = ID_C_STR(item.first);
				str.append(" : " + std::to_string(item.second) + "\n");
				partition_str.append(str);
			}
			FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH", "Partition : %s", partition_str.c_str());

			sync_mode_runtime.getSyncLinker().synchronize();

			// Builds node and arcs export maps
			std::vector<NodeType*> exported_nodes;
			std::unordered_map<int, std::vector<NodePtrWrapper<T>>> node_export_map;
			std::unordered_map<int, std::set<DistributedId>> arc_ids_to_export;
			std::unordered_map<int, std::vector<ArcPtrWrapper<T>>> arc_export_map;
			for(auto item : partition) {
				if(this->getNodes().count(item.first) > 0) {
					if(item.second != mpi_communicator.getRank()) {
						FPMAS_LOGV(getMpiCommunicator().getRank(), "DIST_GRAPH",
								"Exporting node %s to %i", ID_C_STR(item.first), item.second);
						auto node_to_export = this->getNode(item.first);
						exported_nodes.push_back(node_to_export);
						node_export_map[item.second].emplace_back(node_to_export);
						for(auto arc :  node_to_export->getIncomingArcs()) {
							// Insert or replace in the IDs set
							arc_ids_to_export[item.second].insert(arc->getId());
						}
						for(auto arc :  node_to_export->getOutgoingArcs()) {
							// Insert or replace in the IDs set
							arc_ids_to_export[item.second].insert(arc->getId());
						}
					}
				}
			}
			// Ensures that each arc is exported once to each process
			for(auto list : arc_ids_to_export) {
				for(auto id : list.second) {
					arc_export_map[list.first].emplace_back(this->getArc(id));
				}
			}

			// Serialize and export / import nodes
			auto node_import = node_mpi.migrate(node_export_map);

			// Serialize and export / import arcs
			auto arc_import = arc_mpi.migrate(arc_export_map);

			NodeMap imported_nodes;
			for(auto& import_node_list_from_proc : node_import) {
				for(auto& imported_node : import_node_list_from_proc.second) {
					auto node = this->importNode(imported_node);
					imported_nodes.insert({node->getId(), node});
				}
			}
			for(auto& import_arc_list_from_proc : arc_import) {
				for(auto& imported_arc : import_arc_list_from_proc.second) {
					this->importArc(imported_arc);
				}
			}

			for(auto node : exported_nodes) {
				setDistant(node);
			}

			location_manager.updateLocations(imported_nodes);

			FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Clearing exported nodes...");
			for(auto node : exported_nodes) {
				clearNode(node);
			}
			FPMAS_LOGD(getMpiCommunicator().getRank(), "DIST_GRAPH", "Exported nodes cleared.");

			sync_mode_runtime.getDataSync().synchronize();
			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"End of distribution.");
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::synchronize() {
			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"Synchronizing graph...");

			sync_mode_runtime.getSyncLinker().synchronize();

			sync_mode_runtime.getDataSync().synchronize();

			FPMAS_LOGI(getMpiCommunicator().getRank(), "DIST_GRAPH",
					"End of graph synchronization.");
		}

	template<DIST_GRAPH_PARAMS>
		void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clearArc(ArcType* arc) {
			auto src = arc->getSourceNode();
			auto tgt = arc->getTargetNode();
			this->erase(arc);
			if(src->state() == LocationState::DISTANT) {
				this->clearNode(src);
			}
			if(tgt->state() == LocationState::DISTANT) {
				this->clearNode(tgt);
			}
		}

	template<DIST_GRAPH_PARAMS>
	void DistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clearNode(NodeType* node) {
			DistributedId id = node->getId();
			FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Clearing node %s...",
					ID_C_STR(id)
					);

			bool eraseNode = true;
			std::set<ArcType*> obsoleteArcs;
			for(auto arc : node->getIncomingArcs()) {
				if(arc->getSourceNode()->state()==LocationState::LOCAL) {
					eraseNode = false;
				} else {
					obsoleteArcs.insert(arc);
				}
			}
			for(auto arc : node->getOutgoingArcs()) {
				if(arc->getTargetNode()->state()==LocationState::LOCAL) {
					eraseNode = false;
				} else {
					obsoleteArcs.insert(arc);
				}
			}
			if(eraseNode) {
				FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Erasing obsolete node %s.",
					ID_C_STR(node->getId())
					);
				location_manager.remove(node);
				this->erase(node);
			} else {
				for(auto arc : obsoleteArcs) {
					FPMAS_LOGD(
						mpi_communicator.getRank(),
						"DIST_GRAPH", "Erasing obsolete arc %s (%p) (from %s to %s)",
						ID_C_STR(node->getId()), arc,
						ID_C_STR(arc->getSourceNode()->getId()),
						ID_C_STR(arc->getTargetNode()->getId())
						);
					this->erase(arc);
				}
			}
			FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Node %s cleared.",
					ID_C_STR(id)
					);
		}

	template<DIST_GRAPH_PARAMS>
		DistributedGraph<DIST_GRAPH_PARAMS_SPEC>::~DistributedGraph() {
			for(auto callback : set_local_callbacks)
				delete callback;
			for(auto callback : set_distant_callbacks)
				delete callback;
		}
}
#endif
