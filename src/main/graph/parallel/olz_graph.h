#ifndef OLZ_GRAPH_H
#define OLZ_GRAPH_H

#include<string>
#include<unordered_map>

#include "communication/communication.h"
#include "utils/config.h"

#include "olz.h"
#include "../base/node.h"

#include "zoltan/zoltan_ghost_node_migrate.h"

using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::base::FossilArcs;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	template<class T, SYNC_MODE, typename LayerType, int N> class DistributedGraph;

	template<NODE_PARAMS, SYNC_MODE> class GhostNode;
	template<NODE_PARAMS, SYNC_MODE> class GhostArc;

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
	template<NODE_PARAMS, SYNC_MODE> class GhostGraph {
		friend GhostArc<NODE_PARAMS_SPEC, S>;
		friend void zoltan::ghost::obj_size_multi_fn<NODE_PARAMS_SPEC, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
		friend void zoltan::ghost::pack_obj_multi_fn<NODE_PARAMS_SPEC, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
		friend void zoltan::ghost::unpack_obj_multi_fn<NODE_PARAMS_SPEC, S>(
				void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
				);
		private:
		DistributedGraph<T, S, LayerType, N>* localGraph;
		MpiCommunicator mpiCommunicator;
		Zoltan zoltan;

		void initialize();

		std::unordered_map<unsigned long, std::string> ghost_node_serialization_cache;
		std::unordered_map<unsigned long, GhostNode<NODE_PARAMS_SPEC, S>*> ghostNodes;
		std::unordered_map<unsigned long, GhostArc<NODE_PARAMS_SPEC, S>*> ghostArcs;

		public:

		GhostGraph(DistributedGraph<T, S, LayerType, N>*);
		GhostGraph(DistributedGraph<T, S, LayerType, N>*, std::initializer_list<int>);

		void synchronize();

		GhostNode<NODE_PARAMS_SPEC, S>* buildNode(unsigned long);
		GhostNode<NODE_PARAMS_SPEC, S>* buildNode(Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N> node, std::set<unsigned long> ignoreIds = std::set<unsigned long>());

		void link(GhostNode<NODE_PARAMS_SPEC, S>*, Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>*, unsigned long, LayerType);
		void link(Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>*, GhostNode<NODE_PARAMS_SPEC, S>*, unsigned long, LayerType);

		void removeNode(unsigned long);

		std::unordered_map<unsigned long, GhostNode<NODE_PARAMS_SPEC, S>*> getNodes();
		const GhostNode<NODE_PARAMS_SPEC, S>* getNode(unsigned long) const;
		std::unordered_map<unsigned long, GhostArc<NODE_PARAMS_SPEC, S>*> getArcs();
		const GhostArc<NODE_PARAMS_SPEC, S>* getArc(unsigned long) const;

		void clear(FossilArcs<Arc<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>>);

		~GhostGraph();

	};

	/*
	 * Initializes zoltan parameters and zoltan ghost migration query functions.
	 */
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::initialize() {
		FPMAS::config::zoltan_config(&this->zoltan);

		zoltan.Set_Obj_Size_Multi_Fn(zoltan::ghost::obj_size_multi_fn<NODE_PARAMS_SPEC, S>, localGraph);
		zoltan.Set_Pack_Obj_Multi_Fn(zoltan::ghost::pack_obj_multi_fn<NODE_PARAMS_SPEC, S>, localGraph);
		zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::ghost::unpack_obj_multi_fn<NODE_PARAMS_SPEC, S>, localGraph);
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
	template<NODE_PARAMS, SYNC_MODE> GhostGraph<NODE_PARAMS_SPEC, S>::GhostGraph(
			DistributedGraph<T, S, LayerType, N>* localGraph
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
	template<NODE_PARAMS, SYNC_MODE> GhostGraph<NODE_PARAMS_SPEC, S>::GhostGraph(
			DistributedGraph<T, S, LayerType, N>* localGraph, std::initializer_list<int> ranks
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
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::synchronize() {
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
	template<NODE_PARAMS, SYNC_MODE> GhostNode<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>::buildNode(unsigned long id) {
		// Copy the gNode from the original node, including arcs data
		GhostNode<NODE_PARAMS_SPEC, S>* gNode = new GhostNode<NODE_PARAMS_SPEC, S>(
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
	template<NODE_PARAMS, SYNC_MODE> GhostNode<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>
		::buildNode(Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N> node, std::set<unsigned long> ignoreIds) {
		// Builds the gNode from the original node data
		GhostNode<NODE_PARAMS_SPEC, S>* gNode = new GhostNode<NODE_PARAMS_SPEC, S>(
				this->localGraph->getMpiCommunicator(),
				this->localGraph->getProxy(),
				node
				);
		// Register the new GhostNode
		this->ghostNodes[gNode->getId()] = gNode;

		// Replaces the incomingArcs list by proper GhostArcs
		// gNode->incomingArcs.clear();	
		for(auto arc : node.getIncomingArcs()) {
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

		// Replaces the outgoingArcs list by proper GhostArcs
		// gNode->outgoingArcs.clear();
		for(auto arc : node.getOutgoingArcs()) {
			auto localTargetNode = arc->getTargetNode();
			// Same as above
			if(ignoreIds.count(localTargetNode->getId()) == 0
					&& this->getNodes().count(localTargetNode->getId()) == 0)
				this->link(gNode, localTargetNode, arc->getId(), arc->layer);
		}

		return gNode;

	}

	/**
	 * Links the specified nodes with a GhostArc.
	 */ 
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::link(
			GhostNode<NODE_PARAMS_SPEC, S>* source,
			Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>* target,
			unsigned long arc_id,
			LayerType layer
			) {
		this->ghostArcs[arc_id] =
			new GhostArc<NODE_PARAMS_SPEC, S>(arc_id, source, target, layer);
	}

	/**
	 * Links the specified nodes with a GhostArc.
	 */ 
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::link(
			Node<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>* source,
			GhostNode<NODE_PARAMS_SPEC, S>* target,
			unsigned long arc_id,
			LayerType layer
			) {
		this->ghostArcs[arc_id] =
			new GhostArc<NODE_PARAMS_SPEC, S>(arc_id, source, target, layer);
	}

	/**
	 * Removes and deletes the node corresponding to the specified `nodeId` from this
	 * graph, along with associated arcs.
	 *
	 * @param nodeId id of the node to remove
	 */
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::removeNode(unsigned long nodeId) {
		GhostNode<NODE_PARAMS_SPEC, S>* nodeToRemove = this->ghostNodes.at(nodeId);
		FossilArcs<Arc<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>> fossil;
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
	template<NODE_PARAMS, SYNC_MODE> std::unordered_map<unsigned long, GhostNode<NODE_PARAMS_SPEC, S>*> GhostGraph<NODE_PARAMS_SPEC, S>::getNodes() {
		return this->ghostNodes;
	}

	/**
	 * Returns a const pointer to the GhostNode corresponding to the specified
	 * id.
	 *
	 * @return const pointer to GhostNode
	 */
	template<NODE_PARAMS, SYNC_MODE> const GhostNode<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>::getNode(unsigned long id) const {
		return this->ghostNodes.at(id);
	}

	/**
	 * Returns a const pointer to the GhostArc corresponding to the specified
	 * id.
	 *
	 * @return const pointer to GhostArc
	 */
	template<NODE_PARAMS, SYNC_MODE> const GhostArc<NODE_PARAMS_SPEC, S>* GhostGraph<NODE_PARAMS_SPEC, S>::getArc(unsigned long id) const {
		return this->ghostArcs.at(id);
	}

	/**
	 * Returns the map of GhostArc s currently living in this graph.
	 *
	 * @return ghost arcs
	 */
	template<NODE_PARAMS, SYNC_MODE> std::unordered_map<unsigned long, GhostArc<NODE_PARAMS_SPEC, S>*> GhostGraph<NODE_PARAMS_SPEC, S>::getArcs() {
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
	template<NODE_PARAMS, SYNC_MODE> void GhostGraph<NODE_PARAMS_SPEC, S>::clear(FossilArcs<Arc<SyncDataPtr<NODE_PARAMS_SPEC>, LayerType, N>> fossil) {
		for(auto arc : fossil.incomingArcs) {
			// Source node should be a ghost
			GhostNode<NODE_PARAMS_SPEC, S>* ghost = (GhostNode<NODE_PARAMS_SPEC, S>*) arc->getSourceNode();
			if(ghost->getIncomingArcs().size() == 0
					&& ghost->getOutgoingArcs().size() == 0) {
				this->ghostNodes.erase(ghost->getId());
				delete ghost;
			}
			this->ghostArcs.erase(arc->getId());
			delete (GhostArc<NODE_PARAMS_SPEC, S>*) arc;
		}
		for(auto arc : fossil.outgoingArcs) {
			// Target node should be a ghost
			GhostNode<NODE_PARAMS_SPEC, S>* ghost = (GhostNode<NODE_PARAMS_SPEC, S>*) arc->getTargetNode();
			if(ghost->getIncomingArcs().size() == 0
					&& ghost->getOutgoingArcs().size() == 0) {
				this->ghostNodes.erase(ghost->getId());
				delete ghost;
			}
			this->ghostArcs.erase(arc->getId());
			delete (GhostArc<NODE_PARAMS_SPEC, S>*) arc;
		}
	}

	/**
	 * Deletes ghost nodes and arcs remaining in this graph.
	 */
	template<NODE_PARAMS, SYNC_MODE> GhostGraph<NODE_PARAMS_SPEC, S>::~GhostGraph() {
		for(auto node : this->ghostNodes) {
			delete node.second;
		}
		for(auto arc : this->ghostArcs) {
			delete arc.second;
		}
	}

}
#endif
