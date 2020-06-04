#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "api/graph/parallel/distributed_graph.h"
#include "communication/communication.h"
#include "distributed_node.h"
#include "location_manager.h"

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
	
	typedef api::communication::MpiSetUp<communication::MpiCommunicator, communication::TypedMpi> DefaultMpiSetUp;

	template<DIST_GRAPH_PARAMS>
	class BasicDistributedGraph : 
		public base::AbstractGraphBase<
			api::graph::parallel::DistributedNode<T>,
			api::graph::parallel::DistributedArc<T>>,
		public api::graph::parallel::DistributedGraph<T>
		 {
			public:
			typedef DistNodeImpl<T, SyncMode::template MutexType> DistNodeType;
			typedef DistArcImpl<T, SyncMode::template MutexType> DistArcType;
			typedef typename SyncMode::template SyncModeRuntimeType<T, DistNodeType, DistArcType> SyncModeRuntimeType;

			static_assert(
					std::is_base_of<api::graph::parallel::DistributedNode<T>, DistNodeType>::value,
					"DistNodeImpl must implement api::graph::parallel::DistributedNode"
					);
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedArc<T>, DistArcType>::value,
					"DistArcImpl must implement api::graph::parallel::DistributedArc"
					);
			typedef base::AbstractGraphBase<api::graph::parallel::DistributedNode<T>, api::graph::parallel::DistributedArc<T>>
			AbstractGraphBase;
			typedef api::graph::parallel::DistributedGraph<T> DistGraphBase;

			public:
			using typename DistGraphBase::NodeType;
			using typename DistGraphBase::NodeBase;
			using typename DistGraphBase::ArcType;
			using typename DistGraphBase::ArcBase;
			using typename DistGraphBase::LayerIdType;
			using typename DistGraphBase::NodeMap;
			using typename DistGraphBase::PartitionMap;
			typedef typename MpiSetUp::communicator communicator;
			template<typename Data>
			using mpi = typename MpiSetUp::template mpi<Data>;

			private:
			communicator mpi_communicator;
			mpi<DistNodeType> nodeMpi {mpi_communicator};
			mpi<DistArcType> arcMpi {mpi_communicator};

			LocationManagerImpl<NodeType> location_manager;
			SyncModeRuntimeType sync_mode_runtime;
			LoadBalancingImpl<NodeType> load_balancing;


			public:
			BasicDistributedGraph() : location_manager(mpi_communicator), sync_mode_runtime(*this, mpi_communicator) {
				// Initialization in the body of this (derived) class of the
				// (base) fields nodeId and arcId, to ensure that
				// mpi_communicator is initialized (as a field of this derived
				// class)
				this->node_id = DistributedId(mpi_communicator.getRank(), 0);
				this->arc_id = DistributedId(mpi_communicator.getRank(), 0);
			}

			const communicator& getMpiCommunicator() const {
				return mpi_communicator;
			};
			communicator& getMpiCommunicator() {
				return mpi_communicator;
			};

			const mpi<DistNodeType>& getNodeMpi() const {return nodeMpi;}
			const mpi<DistArcType>& getArcMpi() const {return arcMpi;}

			void clear(ArcType*) override;
			void clear(NodeType*) override;

			NodeType* importNode(const NodeType& node) override;
			ArcType* importArc(const ArcType& arc) override;

			const SyncModeRuntimeType& getSyncModeRuntime() const {return sync_mode_runtime;}

			const LoadBalancingImpl<NodeType>& getLoadBalancing() const {
				return load_balancing;
			};

			const LocationManagerImpl<NodeType>&
				getLocationManager() const override {return location_manager;}

			void removeNode(NodeType*) override {};
			void unlink(ArcType*) override;

			void balance() override {
				this->distribute(load_balancing.balance(this->getNodes(), {}));
			};

			void distribute(PartitionMap partition) override;

			void synchronize() override;

			template<typename... Args> NodeType* buildNode(Args... args) {
				NodeType* node = new DistNodeType(
						this->node_id++,
						std::forward<Args>(args)...
						);
				this->insert(node);
				location_manager.setLocal(node);
				location_manager.addManagedNode(node, mpi_communicator.getRank());
				sync_mode_runtime.setUp(node->getId(), dynamic_cast<typename SyncMode::template MutexType<T>&>(node->mutex()));
				return node;
			}

			template<typename... Args> ArcType* link(
					NodeBase* const src, NodeBase* const tgt, LayerIdType layer,
					Args... args) {
				// Locks source and target
				src->mutex().lock();
				tgt->mutex().lock();

				// Builds the new arc
				auto arc = new DistArcType(
						this->arc_id++, layer,
						std::forward<Args>(args)...
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
		};

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::unlink(ArcType* arc) {
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
		this->clear(arc);
	}

	template<DIST_GRAPH_PARAMS>
	typename BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::NodeType*
	BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(
			const NodeType& node) {
		if(this->getNodes().count(node.getId())==0) {
			auto nodeCopy = new DistNodeType(static_cast<const DistNodeType&>(node));
			this->insert(nodeCopy);
			location_manager.setLocal(nodeCopy);
			sync_mode_runtime.setUp(nodeCopy->getId(), nodeCopy->mutex());
			return nodeCopy;
		}
		auto localNode = this->getNode(node.getId());
		location_manager.setLocal(localNode);
		return localNode;
	}

	template<DIST_GRAPH_PARAMS>
	typename BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::ArcType*
	BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importArc(
			const ArcType& arc) {
		if(this->getArcs().count(arc.getId())==0) {
			DistributedId srcId = arc.getSourceNode()->getId();
			NodeType* src;
			DistributedId tgtId =  arc.getTargetNode()->getId();
			NodeType* tgt;
			LocationState arcLocationState = LocationState::LOCAL;
			if(this->getNodes().count(srcId) > 0) {
				src = this->getNode(srcId);
				if(src->state() == LocationState::DISTANT) {
					arcLocationState = LocationState::DISTANT;
				}
			} else {
				arcLocationState = LocationState::DISTANT;
				src = new DistNodeType(srcId);
				this->insert(src);
				location_manager.setDistant(src);
				sync_mode_runtime.setUp(src->getId(), dynamic_cast<typename SyncMode::template MutexType<T>&>(src->mutex()));
			}
			if(this->getNodes().count(tgtId) > 0) {
				tgt = this->getNode(tgtId);
				if(tgt->state() == LocationState::DISTANT) {
					arcLocationState = LocationState::DISTANT;
				}
			} else {
				arcLocationState = LocationState::DISTANT;
				tgt = new DistNodeType(tgtId);
				this->insert(tgt);
				location_manager.setDistant(tgt);
				sync_mode_runtime.setUp(tgt->getId(), dynamic_cast<typename SyncMode::template MutexType<T>&>(tgt->mutex()));
			}
			// TODO : ghosts creation part is nice, but this is not
			// because it can't adapt to any DistArcImpl type, using a generic
			// copy constructor.
			auto arcCopy = new DistArcType(static_cast<const DistArcType&>(arc));
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
		::distribute(PartitionMap partition) {

			sync_mode_runtime.getSyncLinker().synchronize();

			// Builds node and arcs export maps
			std::vector<NodeType*> exportedNodes;
			std::unordered_map<int, std::vector<DistNodeType>> nodeExportMap;
			std::unordered_map<int, std::set<DistributedId>> arcIdsToExport;
			std::unordered_map<int, std::vector<DistArcType>> arcExportMap;
			for(auto item : partition) {
				if(this->getNodes().count(item.first) > 0) {
					if(item.second != mpi_communicator.getRank()) {
						auto nodeToExport = this->getNode(item.first);
						exportedNodes.push_back(nodeToExport);
						nodeExportMap[item.second].push_back(*static_cast<DistNodeType*>(nodeToExport));
						for(auto arc :  nodeToExport->getIncomingArcs()) {
							// Insert or replace in the IDs set
							arcIdsToExport[item.second].insert(arc->getId());
						}
						for(auto arc :  nodeToExport->getOutgoingArcs()) {
							// Insert or replace in the IDs set
							arcIdsToExport[item.second].insert(arc->getId());
						}
					}
				}
			}
			// Ensures that each arc is exported once to each process
			for(auto list : arcIdsToExport) {
				for(auto id : list.second) {
					arcExportMap[list.first].push_back(*static_cast<DistArcType*>(this->getArc(id)));
				}
			}

			// Serialize and export / import data
			auto nodeImport = nodeMpi.migrate(nodeExportMap);

			auto arcImport = arcMpi.migrate(arcExportMap);

			NodeMap importedNodes;
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
				location_manager.setDistant(node);
			}
			for(auto node : exportedNodes) {
				FPMAS_LOGD(
					mpi_communicator.getRank(),
					"DIST_GRAPH", "Clear node %s",
					ID_C_STR(node->getId())
					);
				clear(node);
			}
			location_manager.updateLocations(importedNodes);
			sync_mode_runtime.getDataSync().synchronize();
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::synchronize() {
			sync_mode_runtime.getSyncLinker().synchronize();
			sync_mode_runtime.getDataSync().synchronize();
		}

	template<DIST_GRAPH_PARAMS>
		void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clear(ArcType* arc) {
			auto src = arc->getSourceNode();
			auto tgt = arc->getTargetNode();
			this->erase(arc);
			if(src->state() == LocationState::DISTANT) {
				this->clear(src);
			}
			if(tgt->state() == LocationState::DISTANT) {
				this->clear(tgt);
			}
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clear(NodeType* node) {
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
					"DIST_GRAPH", "Erasing obsolete node %s",
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
		}
}
#endif
