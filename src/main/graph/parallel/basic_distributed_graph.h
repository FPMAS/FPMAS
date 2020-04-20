#ifndef DISTRIBUTED_GRAPH_IMPL_H
#define DISTRIBUTED_GRAPH_IMPL_H

#include "api/communication/communication.h"
#include "api/graph/parallel/distributed_graph.h"

#include "graph/base/basic_graph.h"

#define DIST_GRAPH_PARAMS\
	typename DistNodeImpl,\
	typename DistArcImpl,\
	typename MpiCommunicatorImpl,\
	template<typename> class LoadBalancingImpl

#define DIST_GRAPH_PARAMS_SPEC\
	DistNodeImpl,\
	DistArcImpl,\
	MpiCommunicatorImpl,\
	LoadBalancingImpl

namespace FPMAS::graph::parallel {
	
	using FPMAS::api::graph::parallel::LocationState;
	template<DIST_GRAPH_PARAMS>
	class BasicDistributedGraph : 
		public base::AbstractGraphBase<
			BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>, DistNodeImpl, DistArcImpl>,
		public api::graph::parallel::DistributedGraph<
			BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>, DistNodeImpl, DistArcImpl
		> {
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedNode<typename DistNodeImpl::data_type, DistArcImpl>, DistNodeImpl>
					::value, "DistNodeImpl must implement api::graph::parallel::DistributedNode"
					);
			static_assert(
					std::is_base_of<api::graph::parallel::DistributedArc<typename DistNodeImpl::data_type, DistNodeImpl>, DistArcImpl>
					::value, "DistArcImpl must implement api::graph::parallel::DistributedArc"
					);
			typedef base::AbstractGraphBase<
			BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>, DistNodeImpl, DistArcImpl>
			abstract_graph_base;
			typedef api::graph::parallel::DistributedGraph<
				BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>, DistNodeImpl, DistArcImpl
			> dist_graph_base;

			public:
			using typename dist_graph_base::node_type;
			using typename dist_graph_base::node_base;
			using typename dist_graph_base::arc_type;
			using typename dist_graph_base::arc_base;
			using typename dist_graph_base::layer_id_type;
			using typename dist_graph_base::node_map;
			using typename dist_graph_base::partition_type;

			private:
			LoadBalancingImpl<node_type> loadBalancing;
			MpiCommunicatorImpl mpiCommunicator;

			node_map localNodes;

			void setLocal(node_type*);
			void setDistant(node_type*);
			void clear(node_type*);

			public:
			BasicDistributedGraph() {
				// Initialization in the body of this (derived) class of the
				// (base) fields nodeId and arcId, to ensure that
				// mpiCommunicator is initialized (as a field of this derived
				// class)
				this->nodeId = DistributedId(mpiCommunicator.getRank(), 0);
				this->arcId = DistributedId(mpiCommunicator.getRank(), 0);
			}

			const LoadBalancingImpl<node_type>& getLoadBalancing() const {
				return loadBalancing;
			};
			const MpiCommunicatorImpl& getMpiCommunicator() const {
				return mpiCommunicator;
			};
			MpiCommunicatorImpl& getMpiCommunicator() {
				return mpiCommunicator;
			};

			void removeNode(node_type*) override {};
			void unlink(arc_type*) override {};

			node_type* importNode(const node_type& node) override;

			arc_type* importArc(const arc_type& arc) override;

			const node_map& getLocalNodes() const override { return localNodes;};

			void balance() override {
				this->distribute(loadBalancing.balance(this->getNodes(), {}));
			};

			void distribute(partition_type partition) override;

			void synchronize() override {};

			template<typename... Args> node_type* buildNode(Args... args) {
				auto node = new node_type(
						this->nodeId++,
						std::forward<Args>(args)...
						);
				setLocal(node);
				this->insert(node);
				return node;
			}

			template<typename... Args> arc_type* link(
					node_base* const src, node_base* const tgt, layer_id_type layer,
					Args... args) {
				auto arc = new arc_type(
						this->arcId++, layer,
						std::forward<Args>(args)...
						);
				arc->setSourceNode(src);
				src->linkOut(arc);
				arc->setTargetNode(tgt);
				tgt->linkIn(arc);
				this->insert(arc);
				return arc;
			}
		};
	template<DIST_GRAPH_PARAMS>
	typename BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::node_type*
	BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>::importNode(
			const node_type& node) {
		if(this->getNodes().count(node.getId())==0) {
			auto nodeCopy = new node_type(node);
			setLocal(nodeCopy);
			this->insert(nodeCopy);
			return nodeCopy;
		}
		auto localNode = this->getNode(node.getId());
		setLocal(localNode);
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
				src->setState(LocationState::DISTANT);
				this->insert(src);
			}
			if(this->getNodes().count(tgtId) > 0) {
				tgt = this->getNode(tgtId);
				if(tgt->state() == LocationState::DISTANT) {
					arcLocationState = LocationState::DISTANT;
				}
			} else {
				arcLocationState = LocationState::DISTANT;
				tgt = new node_type(tgtId);
				tgt->setState(LocationState::DISTANT);
				this->insert(tgt);
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
			auto nodeImport = 
				api::communication::migrate(
						mpiCommunicator, nodeExportMap);

			auto arcImport =
				api::communication::migrate(
						mpiCommunicator, arcExportMap);

			for(auto& importNodeListFromProc : nodeImport) {
				for(auto& importedNode : importNodeListFromProc.second) {
					this->importNode(importedNode);
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
				setDistant(node);
			}
			for(auto node : exportedNodes) {
				clear(node);
			}
			//mpiCommunicator.terminate();
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::setLocal(node_type* node) {
			node->setState(LocationState::LOCAL);
			this->localNodes.insert({node->getId(), node});
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::setDistant(node_type* node) {
			node->setState(LocationState::DISTANT);
			this->localNodes.erase(node->getId());
			for(auto arc : node->getOutgoingArcs()) {
				arc->setState(LocationState::DISTANT);
			}
			for(auto arc : node->getIncomingArcs()) {
				arc->setState(LocationState::DISTANT);
			}
		}

	template<DIST_GRAPH_PARAMS>
	void BasicDistributedGraph<DIST_GRAPH_PARAMS_SPEC>
		::clear(node_type* node) {
			bool eraseNode = true;
			for(auto neighbor : node->inNeighbors()) {
				if(neighbor->state()==LocationState::LOCAL) {
					return;
				}
			}
			for(auto neighbor : node->outNeighbors()) {
				if(neighbor->state()==LocationState::LOCAL) {
					return;
				}
			}
			this->erase(node);
		}
}
#endif
