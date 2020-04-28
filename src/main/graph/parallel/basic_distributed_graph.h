#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "api/communication/communication.h"
#include "api/graph/parallel/distributed_graph.h"

#include "graph/base/basic_graph.h"

#define DIST_GRAPH_PARAMS\
	typename T,\
	typename SyncMode,\
	template<typename, template<typename> class> class DistNodeImpl,\
	template<typename, template<typename> class> class DistArcImpl,\
	typename MpiSetUp,\
	template<typename> class LocationManagerImpl,\
	template<typename> class LoadBalancingImpl

#define DIST_GRAPH_PARAMS_SPEC\
	T,\
	SyncMode,\
	DistNodeImpl,\
	DistArcImpl,\
	MpiSetUp,\
	LocationManagerImpl,\
	LoadBalancingImpl

namespace FPMAS::graph::parallel {
	
	using FPMAS::api::graph::parallel::LocationState;
	template<DIST_GRAPH_PARAMS>
	class BasicDistributedGraph : 
		public base::AbstractGraphBase<
			DistNodeImpl<T, SyncMode::template mutex_type>,
			DistArcImpl<T, SyncMode::template mutex_type>>,
		public api::graph::parallel::DistributedGraph<
			DistNodeImpl<T, SyncMode::template mutex_type>,
			DistArcImpl<T, SyncMode::template mutex_type>
		> {
			typedef DistNodeImpl<T, SyncMode::template mutex_type> dist_node_type;
			typedef DistArcImpl<T, SyncMode::template mutex_type> dist_arc_type;
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedNode<
					typename dist_node_type::data_type,
					dist_arc_type
					>, dist_node_type>::value,
					"DistNodeImpl must implement api::graph::parallel::DistributedNode"
					);
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedArc<
					typename dist_node_type::data_type, dist_node_type
					>, dist_arc_type>::value,
					"DistArcImpl must implement api::graph::parallel::DistributedArc"
					);
			typedef base::AbstractGraphBase<dist_node_type, dist_arc_type>
			abstract_graph_base;
			typedef api::graph::parallel::DistributedGraph<dist_node_type, dist_arc_type
			> dist_graph_base;

			public:
			using typename dist_graph_base::node_type;
			using typename dist_graph_base::node_base;
			using typename dist_graph_base::arc_type;
			using typename dist_graph_base::arc_base;
			using typename dist_graph_base::layer_id_type;
			using typename dist_graph_base::node_map;
			using typename dist_graph_base::partition_type;
			typedef typename SyncMode::template sync_linker<node_type, arc_type> sync_linker_type;
			typedef typename SyncMode::template data_sync<node_type, arc_type> data_sync_type;
			typedef typename MpiSetUp::communicator communicator;
			template<typename Data>
			using mpi = typename MpiSetUp::template mpi<Data>;

			private:
			communicator mpiCommunicator;
			mpi<node_type> nodeMpi {mpiCommunicator};
			mpi<arc_type> arcMpi {mpiCommunicator};

			SyncMode syncMode;
			sync_linker_type syncLinker;
			data_sync_type dataSync;
			LocationManagerImpl<node_type> locationManager;
			LoadBalancingImpl<node_type> loadBalancing;


			public:
			BasicDistributedGraph() : locationManager(mpiCommunicator), syncLinker(mpiCommunicator) {
				// Initialization in the body of this (derived) class of the
				// (base) fields nodeId and arcId, to ensure that
				// mpiCommunicator is initialized (as a field of this derived
				// class)
				this->nodeId = DistributedId(mpiCommunicator.getRank(), 0);
				this->arcId = DistributedId(mpiCommunicator.getRank(), 0);
			}

			const communicator& getMpiCommunicator() const {
				return mpiCommunicator;
			};
			communicator& getMpiCommunicator() {
				return mpiCommunicator;
			};

			const mpi<node_type>& getNodeMpi() const {return nodeMpi;}
			const mpi<arc_type>& getArcMpi() const {return arcMpi;}

			void clear(node_type*) override;

			node_type* importNode(const node_type& node) override;
			arc_type* importArc(const arc_type& arc) override;

			const SyncMode& getSyncMode() const {return syncMode;}

			const sync_linker_type& getSyncLinker() const {return syncLinker;}

			const data_sync_type& getDataSync() const {return dataSync;}

			const LoadBalancingImpl<node_type>& getLoadBalancing() const {
				return loadBalancing;
			};

			const FPMAS::api::graph::parallel::LocationManager<node_type>&
				getLocationManager() const override {return locationManager;}

			void removeNode(node_type*) override {};
			void unlink(arc_type*) override;

			void balance() override {
				this->distribute(loadBalancing.balance(this->getNodes(), {}));
			};

			void distribute(partition_type partition) override;

			void synchronize() override;

			template<typename... Args> node_type* buildNode(Args... args) {
				auto node = new node_type(
						this->nodeId++,
						std::forward<Args>(args)...
						);
				this->insert(node);
				locationManager.setLocal(node);
				locationManager.addManagedNode(node, mpiCommunicator.getRank());
				return node;
			}

			template<typename... Args> arc_type* link(
					node_base* const src, node_base* const tgt, layer_id_type layer,
					Args... args) {
				// Locks source and target
				src->mutex().lock();
				tgt->mutex().lock();

				// Builds the new arc
				auto arc = new arc_type(
						this->arcId++, layer,
						std::forward<Args>(args)...
						);
				arc->setSourceNode(src);
				src->linkOut(arc);
				arc->setTargetNode(tgt);
				tgt->linkIn(arc);

				// If src and tgt is DISTANT, transmit the request to the
				// SyncLinker, that will handle the request according to its
				// synchronisation policy
				if(src->state() == LocationState::DISTANT || tgt->state() == LocationState::DISTANT) {
					syncLinker.link(arc);
					arc->setState(LocationState::DISTANT);
				} else {
					arc->setState(LocationState::LOCAL);
				}

				// Inserts the arc in the Graph
				this->insert(arc);

				// Unlocks source and target
				src->mutex().unlock();
				tgt->mutex().unlock();

				return arc;
			}
		};

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(arc_type* arc) {
		auto src = arc->getSourceNode();
		auto tgt = arc->getTargetNode();
		src->mutex().lock();
		tgt->mutex().lock();

		if(arc->state() == LocationState::DISTANT) {
			syncLinker.unlink(arc);
			// TODO : src and tgt might be cleared
		}
		this->erase(arc);

		src->mutex().unlock();
		tgt->mutex().unlock();
	}

	template<DIST_GRAPH_PARAMS>
	typename BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::node_type*
	BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(
			const node_type& node) {
		if(this->getNodes().count(node.getId())==0) {
			auto nodeCopy = new node_type(node);
			this->insert(nodeCopy);
			locationManager.setLocal(nodeCopy);
			return nodeCopy;
		}
		auto localNode = this->getNode(node.getId());
		locationManager.setLocal(localNode);
		return localNode;
	}

	template<DIST_GRAPH_PARAMS>
	typename BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::arc_type*
	BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importArc(
			const arc_type& arc) {
		if(this->getArcs().count(arc.getId())==0) {
			DistributedId srcId = arc.getSourceNode()->getId();
			node_type* src;
			DistributedId tgtId =  arc.getTargetNode()->getId();
			node_type* tgt;
			LocationState arcLocationState = LocationState::LOCAL;
			if(this->getNodes().count(srcId) > 0) {
				src = this->getNode(srcId);
				if(src->state() == LocationState::DISTANT) {
					arcLocationState = LocationState::DISTANT;
				}
			} else {
				arcLocationState = LocationState::DISTANT;
				src = new node_type(srcId);
				this->insert(src);
				locationManager.setDistant(src);
			}
			if(this->getNodes().count(tgtId) > 0) {
				tgt = this->getNode(tgtId);
				if(tgt->state() == LocationState::DISTANT) {
					arcLocationState = LocationState::DISTANT;
				}
			} else {
				arcLocationState = LocationState::DISTANT;
				tgt = new node_type(tgtId);
				this->insert(tgt);
				locationManager.setDistant(tgt);
			}
			// TODO : ghosts creation part is nice, but this is not
			// because it can't adapt to any DistArcImpl type, using a generic
			// copy constructor.
			auto arcCopy = new arc_type(arc);
			arcCopy->setState(arcLocationState);

			arcCopy->setSourceNode(src);
			src->linkOut(arcCopy);
			arcCopy->setTargetNode(tgt);
			tgt->linkIn(arcCopy);

			this->insert(arcCopy);
			return arcCopy;
		}
		// In place updates
		auto localArc = this->getArc(arc.getId());
		if(localArc->getSourceNode()->state() == LocationState::LOCAL
				&& localArc->getTargetNode()->state() == LocationState::LOCAL) {
			localArc->setState(LocationState::LOCAL);
		}
		return localArc;
	}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::distribute(partition_type partition) {

			syncLinker.synchronize(*this);

			// Builds node and arcs export maps
			std::vector<node_type*> exportedNodes;
			std::unordered_map<int, std::vector<node_type>> nodeExportMap;
			std::unordered_map<int, std::vector<arc_type>> arcExportMap;
			for(auto item : partition) {
				if(item.second != mpiCommunicator.getRank()) {
					auto nodeToExport = this->getNode(item.first);
					exportedNodes.push_back(nodeToExport);
					nodeExportMap[item.second].push_back(*nodeToExport);
					for(auto arc :  nodeToExport->getIncomingArcs()) {
						// TODO : unefficient, improve this.
						arcExportMap[item.second].push_back(*this->getArc(arc->getId()));
					}
					for(auto arc :  nodeToExport->getOutgoingArcs()) {
						// TODO : unefficient, improve this.
						arcExportMap[item.second].push_back(*this->getArc(arc->getId()));
					}
				}
			}

			// Serialize and export / import data
			auto nodeImport = nodeMpi.migrate(nodeExportMap);

			auto arcImport = arcMpi.migrate(arcExportMap);

			node_map importedNodes;
			for(auto& importNodeListFromProc : nodeImport) {
				for(auto& importedNode : importNodeListFromProc.second) {
					auto node = this->importNode(importedNode);
					importedNodes.insert({node->getId(), node});
				}
			}
			for(auto& importArcListFromProc : arcImport) {
				for(const auto& importedArc : importArcListFromProc.second) {
					this->importArc(importedArc);
					if(importedArc.getSourceNode() == importedArc.getTargetNode()) {
						delete importedArc.getSourceNode();
					} else {
						delete importedArc.getSourceNode();
						delete importedArc.getTargetNode();
					}
				}
			}

			for(auto node : exportedNodes) {
				locationManager.setDistant(node);
			}
			for(auto node : exportedNodes) {
				FPMAS_LOGD(
					mpiCommunicator.getRank(),
					"DIST_GRAPH", "Clear node %s",
					ID_C_STR(node->getId())
					);
				clear(node);
			}
			locationManager.updateLocations(importedNodes);
			dataSync.synchronize(*this);
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::synchronize() {
			syncLinker.synchronize(*this);
			dataSync.synchronize(*this);
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clear(node_type* node) {
			bool eraseNode = true;
			std::set<arc_type*> obsoleteArcs;
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
					mpiCommunicator.getRank(),
					"DIST_GRAPH", "Erasing obsolete node %s",
					ID_C_STR(node->getId())
					);
				locationManager.remove(node);
				this->erase(node);
			} else {
				for(auto arc : obsoleteArcs) {
					FPMAS_LOGD(
						mpiCommunicator.getRank(),
						"DIST_GRAPH", "Erasing obsolete arc %s (%p) (from %s to %s)",
						ID_C_STR(node->getId()), arc,
						ID_C_STR(arc->getSourceNode()->getId()),
						ID_C_STR(arc->getTargetNode()->getId())
						);
					this->erase(arc);
				}
			}
		}
}
#endif
