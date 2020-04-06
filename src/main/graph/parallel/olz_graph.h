#ifndef OLZ_GRAPH_H
#define OLZ_GRAPH_H

#include<string>
#include<unordered_map>

#include "utils/config.h"

#include "communication/request_handler.h"

#include "olz.h"
#include "../base/node.h"

#include "zoltan/zoltan_ghost_node_migrate.h"

namespace FPMAS::graph::parallel {
	using base::LayerId;
	using base::Arc;
	using base::Graph;

	template<typename T, SYNC_MODE> class DistributedGraphBase;

	template<typename T, SYNC_MODE> class GhostNode;
	template<typename T, SYNC_MODE> class GhostArc;

	/**
	 * A GhostGraph is a data structure used by DistributedGraphBase s to store
	 * and manage GhostNode s and GhostArc s.
	 *
	 * A GhostGraph actually lives within a DistributedGraphBase, because
	 * GhostNode can (and should) be linked to regular nodes of the
	 * DistributedGraphBase thanks to GhostArc s.
	 *
	 * Finally, the GhostGraph can also synchronize GhostNode s data thanks to
	 * the synchronize() function.
	 */
	template<typename T, SYNC_MODE> class GhostGraph {
		friend S<T>;
		friend GhostArc<T, S>;
		friend void zoltan::ghost::obj_size_multi_fn<T, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
		friend void zoltan::ghost::pack_obj_multi_fn<T, S>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
		friend void zoltan::ghost::unpack_obj_multi_fn<T, S>(
				void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
				);
		friend DistributedGraphBase<T,S>;

		private:
		DistributedGraphBase<T, S>* localGraph;
		Zoltan zoltan;

		void initialize();

		std::unordered_map<DistributedId, std::string> ghost_node_serialization_cache;
		std::unordered_map<DistributedId, GhostNode<T, S>*> ghostNodes;
		std::unordered_map<DistributedId, GhostArc<T, S>*> ghostArcs;

		void clear(GhostNode<T,S>*);

		public:

		GhostGraph(DistributedGraphBase<T, S>*);
		//GhostGraph(DistributedGraphBase<T, S>*, std::initializer_list<int>);

		void synchronize();

		// GhostNode constructors
		GhostNode<T, S>* buildNode(DistributedId);
		GhostNode<T, S>* buildNode(
				Node<std::unique_ptr<SyncData<T,S>>, DistributedId>& node,
				std::set<DistributedId> ignoreIds = std::set<DistributedId>()
				);
		// GhostNode destructor
		void removeNode(DistributedId);

		// GhostArc constructors
		GhostArc<T, S>* link(
				GhostNode<T, S>*,
				Node<std::unique_ptr<SyncData<T,S>>, DistributedId>*,
				DistributedId, LayerId
				);
		GhostArc<T, S>* link(
				Node<std::unique_ptr<SyncData<T,S>>, DistributedId>*,
				GhostNode<T, S>*,
				DistributedId, LayerId
				);
		GhostArc<T, S>* link(
				GhostNode<T, S>*, GhostNode<T, S>*,
				DistributedId, LayerId
				);
		// GhostArc destructor
		void unlink(GhostArc<T, S>*);


		// GhostNode getters
		std::unordered_map<DistributedId, GhostNode<T, S>*> getNodes();
		GhostNode<T, S>* getNode(DistributedId);
		const GhostNode<T, S>* getNode(DistributedId) const;

		// GhostArc getters
		std::unordered_map<DistributedId, GhostArc<T, S>*> getArcs();
		GhostArc<T, S>* getArc(DistributedId);
		const GhostArc<T, S>* getArc(DistributedId) const;


		~GhostGraph();

	};

	/*
	 * Initializes zoltan parameters and zoltan ghost migration query functions.
	 */
	template<typename T, SYNC_MODE> void GhostGraph<T, S>::initialize() {
		FPMAS::config::zoltan_config(&this->zoltan);

		zoltan.Set_Obj_Size_Multi_Fn(zoltan::ghost::obj_size_multi_fn<T, S>, localGraph);
		zoltan.Set_Pack_Obj_Multi_Fn(zoltan::ghost::pack_obj_multi_fn<T, S>, localGraph);
		zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::ghost::unpack_obj_multi_fn<T, S>, localGraph);
	}

	/**
	 * Initializes a GhostGraph from the input DistributedGraphBase.
	 *
	 * The DistributedGraphBase instance will be used by the Zoltan query
	 * functions to migrate ghost nodes to fetch data about the local
	 * graph.
	 *
	 * @param localGraph pointer to the origin DistributedGraphBase
	 */
	template<typename T, SYNC_MODE> GhostGraph<T, S>::GhostGraph(
			DistributedGraphBase<T, S>* localGraph
			) : localGraph(localGraph), zoltan(localGraph->getMpiCommunicator().getMpiComm()) {
		this->initialize();
	}

	/**
	 * Initializes a GhostGraph from the input DistributedGraphBase over
	 * the specified procs.
	 *
	 * The DistributedGraphBase instance will be used by the Zoltan query
	 * functions to migrate ghost nodes to fetch data about the local
	 * graph.
	 *
	 * @param localGraph pointer to the origin DistributedGraphBase
	 * @param ranks ranks of the procs on which the GhostGraph is
	 * built
	 */
	/*
	 *template<typename T, SYNC_MODE> GhostGraph<T, S>::GhostGraph(
	 *        DistributedGraphBase<T, S>* localGraph, std::initializer_list<int> ranks
	 *        ) : localGraph(localGraph), (ranks), zoltan(mpiCommunicator.getMpiComm()) {
	 *    this->initialize();
	 *}
	 */

	/**
	 * Synchronizes the GhostNodes contained in this GhostGraph.
	 *
	 * Nodes data and weights are fetched from the procs where each node
	 * currently lives, in consequence the Proxy associated to the local
	 * DistributedGraphBase must also be up to date.
	 *
	 * Arcs creation and deletion are currently **not handled** by this
	 * function.
	 */
	template<typename T, SYNC_MODE> void GhostGraph<T, S>::synchronize() {
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
	template<typename T, SYNC_MODE> GhostNode<T, S>* GhostGraph<T, S>
		::buildNode(DistributedId id) {
		// Copy the gNode from the original node, including arcs data
		GhostNode<T, S>* gNode = new GhostNode<T, S>(
				this->localGraph->getRequestHandler(),
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
	template<typename T, SYNC_MODE> GhostNode<T, S>* GhostGraph<T, S>
		::buildNode(
			Node<std::unique_ptr<SyncData<T,S>>, DistributedId>& node,
			std::set<DistributedId> ignoreIds
			) {
			FPMAS_LOGD(
					this->localGraph->getMpiCommunicator().getRank(),
					"GHOST_GRAPH",
					"Building ghost node %s",
					((std::string) node.getId()).c_str()
					);
			// Builds the gNode from the original node data
			GhostNode<T, S>* gNode = new GhostNode<T, S>(
					this->localGraph->getRequestHandler(),
					this->localGraph->getProxy(),
					node
					);
			// Register the new GhostNode
			this->ghostNodes[gNode->getId()] = gNode;

			for(auto& layer : node.getLayers()) {
				// Replaces the incomingArcs list by proper GhostArcs
				for(auto arc : layer.second.getIncomingArcs()) {
					auto localSourceNode = arc->getSourceNode();
					// Builds the ghost arc if :
					if(
							// The source node is not ignored (i.e. it is exported in the
							// current epoch)
							ignoreIds.count(localSourceNode->getId()) == 0
							// AND it is not an already built ghost node
							&& this->getNodes().count(localSourceNode->getId()) == 0
					  ) {
						FPMAS_LOGV(
								this->localGraph->getMpiCommunicator().getRank(),
								"GHOST_GRAPH",
								"Ghost linking %s : (%s, %s)",
								ID_C_STR(arc->getId()),
								ID_C_STR(localSourceNode->getId()),
								ID_C_STR(gNode->getId())
								);
						this->link(localSourceNode, gNode, arc->getId(), arc->layer);
					}
				}

				// Replaces the outgoingArcs list by proper GhostArcs
				for(auto arc : layer.second.getOutgoingArcs()) {
					auto localTargetNode = arc->getTargetNode();
					// Same as above
					if(ignoreIds.count(localTargetNode->getId()) == 0
							&& this->getNodes().count(localTargetNode->getId()) == 0) {
						FPMAS_LOGV(
								this->localGraph->getMpiCommunicator().getRank(),
								"GHOST_GRAPH",
								"Ghost linking %s : (%s, %s)",
								ID_C_STR(arc->getId()),
								ID_C_STR(gNode->getId()),
								ID_C_STR(localTargetNode->getId())
								);
						this->link(gNode, localTargetNode, arc->getId(), arc->layer);
					}
				}
			}

			return gNode;

		}

	/**
	 * Links the specified nodes with a GhostArc.
	 *
	 * @param source pointer to source GhostNode
	 * @param target pointer to local synchronized target Node
	 * @param arcId id of the ghost arc
	 * @param layer arc layer
	 * @return pointer to the created arc
	 */ 
	template<typename T, SYNC_MODE> GhostArc<T, S>* GhostGraph<T, S>::link(
			GhostNode<T, S>* source,
			Node<std::unique_ptr<SyncData<T,S>>, DistributedId>* target,
			DistributedId arcId,
			LayerId layer
			) {
		return this->ghostArcs[arcId] = new GhostArc<T, S>(arcId, source, target, layer);
	}

	/**
	 * Links the specified nodes with a GhostArc.
	 *
	 * @param source pointer to local synchronized source Node
	 * @param target pointer to target GhostNode
	 * @param arcId id of the ghost arc
	 * @param layer arc layer
	 * @return pointer to the created arc
	 */ 
	template<typename T, SYNC_MODE> GhostArc<T, S>* GhostGraph<T, S>::link(
			Node<std::unique_ptr<SyncData<T,S>>, DistributedId>* source,
			GhostNode<T, S>* target,
			DistributedId arcId,
			LayerId layer
			) {
		return this->ghostArcs[arcId] =
			new GhostArc<T, S>(arcId, source, target, layer);
	}

	/**
	 * Links the specified nodes with a GhostArc.
	 *
	 * @param source pointer to source GhostNode
	 * @param target pointer to target GhostNode
	 * @param arcId id of the ghost arc
	 * @param layer arc layer
	 * @return pointer to the created arc
	 */
	template<typename T, SYNC_MODE> GhostArc<T, S>* GhostGraph<T, S>::link(
			GhostNode<T, S>* source,
			GhostNode<T, S>* target,
			DistributedId arcId,
			LayerId layer) {
		return this->ghostArcs[arcId] =
			new GhostArc<T, S>(arcId, source, target, layer);
	}

	/**
	 * Removes and deletes the node corresponding to the specified `nodeId` from this
	 * graph, along with associated arcs.
	 *
	 * @param nodeId id of the node to remove
	 */
	template<typename T, SYNC_MODE> void GhostGraph<T, S>
		::removeNode(DistributedId nodeId) {
		GhostNode<T, S>* nodeToRemove;
		try {
			nodeToRemove = this->ghostNodes.at(nodeId);
		} catch(std::out_of_range) {
			throw FPMAS::graph::base::exceptions::node_out_of_graph(nodeId);
		}

		for(auto& layer : nodeToRemove->getLayers()) {
			// Deletes incoming ghost arcs
			for(auto arc : layer.second.getIncomingArcs()) {
				arc->unlink();
				this->ghostArcs.erase(arc->getId());
				delete arc;
			}

			// Deletes outgoing arcs
			for(auto arc : layer.second.getOutgoingArcs()) {
				arc->unlink();
				this->ghostArcs.erase(arc->getId());
				delete arc;
			}
		}
		this->ghostNodes.erase(nodeToRemove->getId());
		delete nodeToRemove;
	}

	/**
	 * Returns the map of GhostNode s currently living in this graph.
	 *
	 * @return ghost nodes
	 */
	template<typename T, SYNC_MODE> std::unordered_map<DistributedId, GhostNode<T, S>*> GhostGraph<T, S>::getNodes() {
		return this->ghostNodes;
	}

	/**
	 * Returns a pointer to the GhostNode corresponding to the specified
	 * id.
	 *
	 * @return pointer to GhostNode
	 */
	template<typename T, SYNC_MODE> GhostNode<T, S>* GhostGraph<T, S>::getNode(DistributedId id) {
		return this->ghostNodes.at(id);
	}

	/**
	 * Returns a pointer to the GhostNode corresponding to the specified
	 * id.
	 *
	 * @return pointer to GhostNode
	 */
	template<typename T, SYNC_MODE> const GhostNode<T, S>* GhostGraph<T, S>::getNode(DistributedId id) const {
		return this->ghostNodes.at(id);
	}

	/**
	 * Returns a pointer to the GhostArc corresponding to the specified
	 * id.
	 *
	 * @return pointer to GhostArc
	 */
	template<typename T, SYNC_MODE> GhostArc<T, S>* GhostGraph<T, S>::getArc(DistributedId id) {
		return this->ghostArcs.at(id);
	}

	/**
	 * Returns a pointer to the GhostArc corresponding to the specified
	 * id.
	 *
	 * @return pointer to GhostArc
	 */
	template<typename T, SYNC_MODE> const GhostArc<T, S>* GhostGraph<T, S>::getArc(DistributedId id) const {
		return this->ghostArcs.at(id);
	}

	/**
	 * Returns the map of GhostArc s currently living in this graph.
	 *
	 * @return ghost arcs
	 */
	template<typename T, SYNC_MODE> std::unordered_map<DistributedId, GhostArc<T, S>*> GhostGraph<T, S>::getArcs() {
		return this->ghostArcs;
	}

	/**
	 * Removes the specified GhostNode from the graph if it's _orphan_.
	 *
	 * A GhostNode is considered "orphan" if it is locally completely
	 * unconnected.
	 *
	 * @param ghost pointer to the removed GhostNode
	 */
	template<typename T, SYNC_MODE> void GhostGraph<T, S>
		::clear(GhostNode<T,S>* ghost) {
		bool toDelete = true;
		for(auto& layer : ghost->getLayers()) {
			if(layer.second.getIncomingArcs().size() > 0
					|| layer.second.getOutgoingArcs().size() > 0) {
				toDelete = false;
				break;
			}
		}
		if(toDelete) {
			FPMAS_LOGD(
				this->localGraph->getMpiCommunicator().getRank(),
				"GHOST_GRAPH", "Clearing orphan node %s",
				ID_C_STR(ghost->getId())
				);
			this->ghostNodes.erase(ghost->getId());
			delete ghost;
		}
	}

	/**
	 * Unlinks and deletes the specified arc from the GhostGraph.
	 *
	 * Source and target nodes are eventually removed too if they are _orphan_
	 * after the deletion of this arc.
	 *
	 * @param arc GhostArc to unlink
	 */
	template<typename T, SYNC_MODE> void GhostGraph<T, S>
		::unlink(GhostArc<T, S>* arc) {
		FPMAS_LOGD(
			this->localGraph->getMpiCommunicator().getRank(),
			"GHOST_GRAPH", "Unlinking ghost arc %s",
			ID_C_STR(arc->getId())
			);
		arc->unlink();
		if(this->localGraph->getNodes().count(arc->getSourceNode()->getId()) == 0) {
			this->clear((GhostNode<T,S>*) arc->getSourceNode());
		}
		if(this->localGraph->getNodes().count(arc->getTargetNode()->getId()) == 0) {
			this->clear((GhostNode<T,S>*) arc->getTargetNode());
		}
		this->ghostArcs.erase(arc->getId());
		delete arc;
	}
	
	/**
	 * Deletes ghost nodes and arcs remaining in this graph.
	 */
	template<typename T, SYNC_MODE> GhostGraph<T, S>::~GhostGraph() {
		for(auto node : this->ghostNodes) {
			delete node.second;
		}
		for(auto arc : this->ghostArcs) {
			delete arc.second;
		}
	}

}
#endif
