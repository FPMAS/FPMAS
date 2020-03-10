#ifndef OLZ_GRAPH_H
#define OLZ_GRAPH_H

#include<string>
#include<unordered_map>

#include "utils/config.h"

#include "communication/communication.h"

#include "olz.h"
#include "../base/node.h"

#include "zoltan/zoltan_ghost_node_migrate.h"

using FPMAS::communication::MpiCommunicator;

namespace FPMAS::graph::parallel {
	using base::ArcId;
	using base::LayerId;
	using base::FossilArcs;
	using base::Arc;

	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	template<typename T, int N, SYNC_MODE> class GhostNode;
	template<typename T, int N, SYNC_MODE> class GhostArc;

	/**
	 * A GhostGraph is a data structure used by DistributedGraph s to store
	 * and manage GhostNode s and GhostArc s.
	 *
	 * A GhostGraph actually lives within a DistributedGraph, because
	 * GhostNode can (and should) be linked to regular nodes of the
	 * DistributedGraph thanks to GhostArc s.
	 *
	 * Finally, the GhostGraph can also synchronize GhostNode s data thanks to
	 * the synchronize() function.
	 */
	template<typename T, int N, SYNC_MODE> class GhostGraph {
		friend GhostArc<T, N, S>;
		friend void zoltan::ghost::obj_size_multi_fn<T, N, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
		friend void zoltan::ghost::pack_obj_multi_fn<T, N, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
		friend void zoltan::ghost::unpack_obj_multi_fn<T, N, S>(
				void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
				);
		private:
		DistributedGraph<T, S, N>* localGraph;
		MpiCommunicator mpiCommunicator;
		Zoltan zoltan;

		void initialize();

		std::unordered_map<NodeId, std::string> ghost_node_serialization_cache;
		std::unordered_map<NodeId, GhostNode<T, N, S>*> ghostNodes;
		std::unordered_map<ArcId, GhostArc<T, N, S>*> ghostArcs;

		public:

		GhostGraph(DistributedGraph<T, S, N>*);
		GhostGraph(DistributedGraph<T, S, N>*, std::initializer_list<int>);

		void synchronize();

		GhostNode<T, N, S>* buildNode(NodeId);
		GhostNode<T, N, S>* buildNode(Node<std::unique_ptr<SyncData<T>>, N>& node, std::set<NodeId> ignoreIds = std::set<NodeId>());

		GhostArc<T, N, S>* link(GhostNode<T, N, S>*, Node<std::unique_ptr<SyncData<T>>, N>*, ArcId, LayerId);
		GhostArc<T, N, S>* link(Node<std::unique_ptr<SyncData<T>>, N>*, GhostNode<T, N, S>*, ArcId, LayerId);
		GhostArc<T, N, S>* link(GhostNode<T, N, S>*, GhostNode<T, N, S>*, ArcId, LayerId);

		void removeNode(NodeId);

		std::unordered_map<NodeId, GhostNode<T, N, S>*> getNodes();
		GhostNode<T, N, S>* getNode(NodeId);
		const GhostNode<T, N, S>* getNode(NodeId) const;
		std::unordered_map<ArcId, GhostArc<T, N, S>*> getArcs();
		const GhostArc<T, N, S>* getArc(ArcId) const;

		void clear(FossilArcs<Arc<std::unique_ptr<SyncData<T>>, N>>);

		~GhostGraph();

	};

	/*
	 * Initializes zoltan parameters and zoltan ghost migration query functions.
	 */
	template<typename T, int N, SYNC_MODE> void GhostGraph<T, N, S>::initialize() {
		FPMAS::config::zoltan_config(&this->zoltan);

		zoltan.Set_Obj_Size_Multi_Fn(zoltan::ghost::obj_size_multi_fn<T, N, S>, localGraph);
		zoltan.Set_Pack_Obj_Multi_Fn(zoltan::ghost::pack_obj_multi_fn<T, N, S>, localGraph);
		zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::ghost::unpack_obj_multi_fn<T, N, S>, localGraph);
	}

	/**
	 * Initializes a GhostGraph from the input DistributedGraph.
	 *
	 * The DistributedGraph instance will be used by the Zoltan query
	 * functions to migrate ghost nodes to fetch data about the local
	 * graph.
	 *
	 * @param localGraph pointer to the origin DistributedGraph
	 */
	template<typename T, int N, SYNC_MODE> GhostGraph<T, N, S>::GhostGraph(
			DistributedGraph<T, S, N>* localGraph
			) : localGraph(localGraph), zoltan(mpiCommunicator.getMpiComm()) {
		this->initialize();
	}

	/**
	 * Initializes a GhostGraph from the input DistributedGraph over
	 * the specified procs.
	 *
	 * The DistributedGraph instance will be used by the Zoltan query
	 * functions to migrate ghost nodes to fetch data about the local
	 * graph.
	 *
	 * @param localGraph pointer to the origin DistributedGraph
	 * @param ranks ranks of the procs on which the GhostGraph is
	 * built
	 */
	template<typename T, int N, SYNC_MODE> GhostGraph<T, N, S>::GhostGraph(
			DistributedGraph<T, S, N>* localGraph, std::initializer_list<int> ranks
			) : localGraph(localGraph), mpiCommunicator(ranks), zoltan(mpiCommunicator.getMpiComm()) {
		this->initialize();
	}

	/**
	 * Synchronizes the GhostNodes contained in this GhostGraph.
	 *
	 * Nodes data and weights are fetched from the procs where each node
	 * currently lives, in consequence the Proxy associated to the local
	 * DistributedGraph must also be up to date.
	 *
	 * Arcs creation and deletion are currently **not handled** by this
	 * function.
	 */
	template<typename T, int N, SYNC_MODE> void GhostGraph<T, N, S>::synchronize() {
		int import_ghosts_num = this->ghostNodes.size(); 
		ZOLTAN_ID_PTR import_ghosts_global_ids
			= (ZOLTAN_ID_PTR) std::malloc(sizeof(ZOLTAN_ID_TYPE) * import_ghosts_num * 2);
		ZOLTAN_ID_PTR import_ghosts_local_ids;
		int* import_ghost_procs
			= (int*) std::malloc(sizeof(int) * import_ghosts_num);

		int i = 0;
		for(auto ghost : ghostNodes) {
			zoltan::utils::write_zoltan_id(ghost.first, &import_ghosts_global_ids[2 * i]);
			import_ghost_procs[i] = this->localGraph->getProxy().getCurrentLocation(ghost.first);
			i++;
		}

		this->zoltan.Migrate(
				import_ghosts_num,
				import_ghosts_global_ids,
				import_ghosts_local_ids,
				import_ghost_procs,
				import_ghost_procs, // proc = part
				-1,
				NULL,
				NULL,
				NULL,
				NULL
				);

		std::free(import_ghosts_global_ids);
		std::free(import_ghost_procs);
	}

	/**
	 * Builds a GhostNode with the specified id and adds it to this ghost
	 * graph.
	 *
	 * @param id node id
	 */
	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>* GhostGraph<T, N, S>::buildNode(NodeId id) {
		// Copy the gNode from the original node, including arcs data
		GhostNode<T, N, S>* gNode = new GhostNode<T, N, S>(
				this->localGraph->getMpiCommunicator(),
				this->localGraph->getProxy(),
				id
				);
		// Register the new GhostNode
		this->ghostNodes[gNode->getId()] = gNode;
		return gNode;
	}

	/**
	 * Builds a GhostNode as an exact copy of the specified node.
	 *
	 * For each incoming/outgoing arcs of the input nodes, the
	 * corresponding source/target nodes are linked to the built GhostNode
	 * with GhostArc s. In other terms, all the arcs linked to the original
	 * node are duplicated.
	 *
	 * A set of node ids to ignore can be used optionally. All the links to
	 * the nodes specified in this set, incoming or outgoing, will be
	 * ignored. For example, this is used when exporting nodes to ignore
	 * links to other nodes that will also be exported, so that no link
	 * between two ghost nodes will be created.
	 *
	 * The original node can then be safely removed from the graph.
	 *
	 * @param node node from which a ghost copy must be created
	 * @param ignoreIds ids of nodes to ignore when building links
	 */
	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>* GhostGraph<T, N, S>
		::buildNode(Node<std::unique_ptr<SyncData<T>>, N>& node, std::set<NodeId> ignoreIds) {
		// Builds the gNode from the original node data
		GhostNode<T, N, S>* gNode = new GhostNode<T, N, S>(
				this->localGraph->getMpiCommunicator(),
				this->localGraph->getProxy(),
				node
				);
		// Register the new GhostNode
		this->ghostNodes[gNode->getId()] = gNode;

		// Replaces the incomingArcs list by proper GhostArcs
		for(auto layer : node.getLayers()) {
			for(auto arc : layer.getIncomingArcs()) {
				auto localSourceNode = arc->getSourceNode();
				// Builds the ghost arc if :
				if(
						// The source node is not ignored (i.e. it is exported in the
						// current epoch)
						ignoreIds.count(localSourceNode->getId()) == 0
						// AND it is not an already built ghost node
						&& this->getNodes().count(localSourceNode->getId()) == 0
				  )
					this->link(localSourceNode, gNode, arc->getId(), arc->layer);
			}
		}

		// Replaces the outgoingArcs list by proper GhostArcs
		for(auto layer : node.getLayers()) {
			for(auto arc : layer.getOutgoingArcs()) {
				auto localTargetNode = arc->getTargetNode();
				// Same as above
				if(ignoreIds.count(localTargetNode->getId()) == 0
						&& this->getNodes().count(localTargetNode->getId()) == 0)
					this->link(gNode, localTargetNode, arc->getId(), arc->layer);
			}
		}

		return gNode;

	}

	/**
	 * Links the specified nodes with a GhostArc.
	 */ 
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
			GhostNode<T, N, S>* source,
			Node<std::unique_ptr<SyncData<T>>, N>* target,
			ArcId arcId,
			LayerId layer
			) {
		return this->ghostArcs[arcId] = new GhostArc<T, N, S>(arcId, source, target, layer);
	}

	/**
	 * Links the specified nodes with a GhostArc.
	 */ 
	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
			Node<std::unique_ptr<SyncData<T>>, N>* source,
			GhostNode<T, N, S>* target,
			ArcId arcId,
			LayerId layer
			) {
		return this->ghostArcs[arcId] =
			new GhostArc<T, N, S>(arcId, source, target, layer);
	}

	template<typename T, int N, SYNC_MODE> GhostArc<T, N, S>* GhostGraph<T, N, S>::link(
			GhostNode<T, N, S>* source,
			GhostNode<T, N, S>* target,
			ArcId arcId,
			LayerId layer) {
		return this->ghostArcs[arcId] =
			new GhostArc<T, N, S>(arcId, source, target, layer);
	}

	/**
	 * Removes and deletes the node corresponding to the specified `nodeId` from this
	 * graph, along with associated arcs.
	 *
	 * @param nodeId id of the node to remove
	 */
	template<typename T, int N, SYNC_MODE> void GhostGraph<T, N, S>::removeNode(NodeId nodeId) {
		GhostNode<T, N, S>* nodeToRemove = this->ghostNodes.at(nodeId);
		FossilArcs<Arc<std::unique_ptr<SyncData<T>>, N>> fossil;
		// Deletes incoming arcs
		for(auto arc : nodeToRemove->getIncomingArcs()) {
			if(!localGraph->unlink(arc))
				fossil.incomingArcs.insert(arc);
		}

		// Deletes outgoing arcs
		for(auto arc : nodeToRemove->getOutgoingArcs()) {
			if(!localGraph->unlink(arc))
				fossil.outgoingArcs.insert(arc);
		}
		this->ghostNodes.erase(nodeToRemove->getId());
		delete nodeToRemove;

		this->clear(fossil);
	}

	/**
	 * Returns the map of GhostNode s currently living in this graph.
	 *
	 * @return ghost nodes
	 */
	template<typename T, int N, SYNC_MODE> std::unordered_map<NodeId, GhostNode<T, N, S>*> GhostGraph<T, N, S>::getNodes() {
		return this->ghostNodes;
	}

	/**
	 * Returns a const pointer to the GhostNode corresponding to the specified
	 * id.
	 *
	 * @return const pointer to GhostNode
	 */
	template<typename T, int N, SYNC_MODE> const GhostNode<T, N, S>* GhostGraph<T, N, S>::getNode(NodeId id) const {
		return this->ghostNodes.at(id);
	}

	template<typename T, int N, SYNC_MODE> GhostNode<T, N, S>* GhostGraph<T, N, S>::getNode(NodeId id) {
		return this->ghostNodes.at(id);
	}

	/**
	 * Returns a const pointer to the GhostArc corresponding to the specified
	 * id.
	 *
	 * @return const pointer to GhostArc
	 */
	template<typename T, int N, SYNC_MODE> const GhostArc<T, N, S>* GhostGraph<T, N, S>::getArc(ArcId id) const {
		return this->ghostArcs.at(id);
	}

	/**
	 * Returns the map of GhostArc s currently living in this graph.
	 *
	 * @return ghost arcs
	 */
	template<typename T, int N, SYNC_MODE> std::unordered_map<ArcId, GhostArc<T, N, S>*> GhostGraph<T, N, S>::getArcs() {
		return this->ghostArcs;
	}

	/**
	 * Remove the arcs contained in the specified fossil from these ghost
	 * and `delete` them, along with the orphan ghost nodes resulting from
	 * the operation.
	 *
	 * A GhostNode is considered orphan when the deletion of one of its
	 * incoming or outgoing arc makes it completely unconnected. Such a
	 * node is useless for the graph structure and should be deleted.
	 *
	 * @param fossil resulting FossilArcs from removeNode operations performed
	 * on the graph, typically when nodes are exported
	 */
	template<typename T, int N, SYNC_MODE> void GhostGraph<T, N, S>::clear(FossilArcs<Arc<std::unique_ptr<SyncData<T>>, N>> fossil) {
		for(auto arc : fossil.incomingArcs) {
			// Source node should be a ghost
			GhostNode<T, N, S>* ghost = (GhostNode<T, N, S>*) arc->getSourceNode();
			if(ghost->getIncomingArcs().size() == 0
					&& ghost->getOutgoingArcs().size() == 0) {
				this->ghostNodes.erase(ghost->getId());
				delete ghost;
			}
			this->ghostArcs.erase(arc->getId());
			delete (GhostArc<T, N, S>*) arc;
		}
		for(auto arc : fossil.outgoingArcs) {
			// Target node should be a ghost
			GhostNode<T, N, S>* ghost = (GhostNode<T, N, S>*) arc->getTargetNode();
			if(ghost->getIncomingArcs().size() == 0
					&& ghost->getOutgoingArcs().size() == 0) {
				this->ghostNodes.erase(ghost->getId());
				delete ghost;
			}
			this->ghostArcs.erase(arc->getId());
			delete (GhostArc<T, N, S>*) arc;
		}
	}

	/**
	 * Deletes ghost nodes and arcs remaining in this graph.
	 */
	template<typename T, int N, SYNC_MODE> GhostGraph<T, N, S>::~GhostGraph() {
		for(auto node : this->ghostNodes) {
			delete node.second;
		}
		for(auto arc : this->ghostArcs) {
			delete arc.second;
		}
	}

}
#endif
