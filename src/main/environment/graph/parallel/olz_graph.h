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

namespace FPMAS {
	namespace graph {

		template<class T, template<typename> class S> class DistributedGraph;

		namespace synchro {

			template<class T, template<typename> class S> class GhostNode;
			template<class T, template<typename> class S> class GhostArc;

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
			template<class T, template<typename> class S = GhostData> class GhostGraph {
				friend GhostArc<T, S>;
				friend void zoltan::ghost::obj_size_multi_fn<T, S>(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
				friend void zoltan::ghost::pack_obj_multi_fn<T, S>(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
				friend void zoltan::ghost::unpack_obj_multi_fn<T, S>(
						void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
						);
				private:
				DistributedGraph<T, S>* localGraph;
				MpiCommunicator mpiCommunicator;
				Zoltan zoltan;

				void initialize();

				std::unordered_map<unsigned long, std::string> ghost_node_serialization_cache;
				std::unordered_map<unsigned long, GhostNode<T, S>*> ghostNodes;
				std::unordered_map<unsigned long, GhostArc<T, S>*> ghostArcs;

				public:

				GhostGraph(DistributedGraph<T, S>*);
				GhostGraph(DistributedGraph<T, S>*, std::initializer_list<int>);

				void synchronize();

				GhostNode<T, S>* buildNode(unsigned long);
				GhostNode<T, S>* buildNode(Node<SyncData<T>> node, std::set<unsigned long> ignoreIds = std::set<unsigned long>());

				void link(GhostNode<T, S>*, Node<SyncData<T>>*, unsigned long);
				void link(Node<SyncData<T>>*, GhostNode<T, S>*, unsigned long);

				void removeNode(unsigned long);

				std::unordered_map<unsigned long, GhostNode<T, S>*> getNodes();
				std::unordered_map<unsigned long, GhostArc<T, S>*> getArcs();

				void clear(FossilArcs<SyncData<T>>);

				~GhostGraph();

			};

			template<class T, template<typename> class S> void GhostGraph<T, S>::initialize() {
				FPMAS::config::zoltan_config(&this->zoltan);

				zoltan.Set_Obj_Size_Multi_Fn(zoltan::ghost::obj_size_multi_fn<T>, localGraph);
				zoltan.Set_Pack_Obj_Multi_Fn(zoltan::ghost::pack_obj_multi_fn<T>, localGraph);
				zoltan.Set_Unpack_Obj_Multi_Fn(zoltan::ghost::unpack_obj_multi_fn<T>, localGraph);
			}

			/**
			 * Initializes a GhostGraph from the input DistributedGraph.
			 *
			 * The DistributedGraph instance will be used by the Zoltan query
			 * functions used to migrate ghost nodes to fetch data about the local
			 * graph.
			 *
			 * @param localGraph pointer to the origin DistributedGraph
			 */
			template<class T, template<typename> class S> GhostGraph<T, S>::GhostGraph(
					DistributedGraph<T, S>* localGraph
					) : localGraph(localGraph), zoltan(mpiCommunicator.getMpiComm()) {
				this->initialize();
			}

			template<class T, template<typename> class S> GhostGraph<T, S>::GhostGraph(
					DistributedGraph<T, S>* localGraph, std::initializer_list<int> ranks
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
			template<class T, template<typename> class S> void GhostGraph<T, S>::synchronize() {
				int import_ghosts_num = this->ghostNodes.size(); 
				ZOLTAN_ID_PTR import_ghosts_global_ids
					= (ZOLTAN_ID_PTR) std::malloc(sizeof(ZOLTAN_ID_TYPE) * import_ghosts_num * 2);
				ZOLTAN_ID_PTR import_ghosts_local_ids;
				int* import_ghost_procs
					= (int*) std::malloc(sizeof(int) * import_ghosts_num);

				int i = 0;
				for(auto ghost : ghostNodes) {
					zoltan::utils::write_zoltan_id(ghost.first, &import_ghosts_global_ids[2 * i]);
					import_ghost_procs[i] = this->localGraph->getProxy()->getCurrentLocation(ghost.first);
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
			template<class T, template<typename> class S> GhostNode<T, S>* GhostGraph<T, S>::buildNode(unsigned long id) {
				// Copy the gNode from the original node, including arcs data
				GhostNode<T, S>* gNode = new GhostNode<T, S>(id);
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
			template<class T, template<typename> class S> GhostNode<T, S>* GhostGraph<T, S>::buildNode(Node<SyncData<T>> node, std::set<unsigned long> ignoreIds) {
				// Copy the gNode from the original node, including arcs data
				GhostNode<T, S>* gNode = new GhostNode<T, S>(node);
				// Register the new GhostNode
				this->ghostNodes[gNode->getId()] = gNode;

				// Replaces the incomingArcs list by proper GhostArcs
				gNode->incomingArcs.clear();	
				for(auto arc : node.getIncomingArcs()) {
					Node<SyncData<T>>* localSourceNode = arc->getSourceNode();
					// Builds the ghost arc if :
					if(
							// The source node is not ignored (i.e. it is exported in the
							// current epoch)
							ignoreIds.count(localSourceNode->getId()) == 0
							// AND it is not an already built ghost node
							&& this->getNodes().count(localSourceNode->getId()) == 0
					  )
						this->link(localSourceNode, gNode, arc->getId());
				}

				// Replaces the outgoingArcs list by proper GhostArcs
				gNode->outgoingArcs.clear();
				for(auto arc : node.getOutgoingArcs()) {
					Node<SyncData<T>>* localTargetNode = arc->getTargetNode();
					// Same as above
					if(ignoreIds.count(localTargetNode->getId()) == 0
							&& this->getNodes().count(localTargetNode->getId()) == 0)
						this->link(gNode, localTargetNode, arc->getId());
				}

				return gNode;

			}

			/**
			 * Links the specified nodes with a GhostArc.
			 */ 
			template<class T, template<typename> class S> void GhostGraph<T, S>::link(GhostNode<T, S>* source, Node<SyncData<T>>* target, unsigned long arc_id) {
				this->ghostArcs[arc_id] =
					new GhostArc<T, S>(arc_id, source, target);
			}

			/**
			 * Links the specified nodes with a GhostArc.
			 */ 
			template<class T, template<typename> class S> void GhostGraph<T, S>::link(Node<SyncData<T>>* source, GhostNode<T, S>* target, unsigned long arc_id) {
				this->ghostArcs[arc_id] =
					new GhostArc<T, S>(arc_id, source, target);
			}

			/**
			 * Removes and deletes the node corresponding to the specified `nodeId` from this
			 * graph, along with associated arcs.
			 *
			 * @param nodeId id of the node to remove
			 */
			template<class T, template<typename> class S> void GhostGraph<T, S>::removeNode(unsigned long nodeId) {
				GhostNode<T, S>* nodeToRemove = this->ghostNodes.at(nodeId);
				FossilArcs<SyncData<T>> fossil;
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
			template<class T, template<typename> class S> std::unordered_map<unsigned long, GhostNode<T, S>*> GhostGraph<T, S>::getNodes() {
				return this->ghostNodes;
			}

			/**
			 * Returns the map of GhostArc s currently living in this graph.
			 *
			 * @return ghost arcs
			 */
			template<class T, template<typename> class S> std::unordered_map<unsigned long, GhostArc<T, S>*> GhostGraph<T, S>::getArcs() {
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
			template<class T, template<typename> class S> void GhostGraph<T, S>::clear(FossilArcs<SyncData<T>> fossil) {
				for(auto arc : fossil.incomingArcs) {
					// Source node should be a ghost
					GhostNode<T, S>* ghost = (GhostNode<T, S>*) arc->getSourceNode();
					if(ghost->getIncomingArcs().size() == 0
							&& ghost->getOutgoingArcs().size() == 0) {
						this->ghostNodes.erase(ghost->getId());
						delete ghost;
					}
					this->ghostArcs.erase(arc->getId());
					delete (GhostArc<T, S>*) arc;
				}
				for(auto arc : fossil.outgoingArcs) {
					// Target node should be a ghost
					GhostNode<T, S>* ghost = (GhostNode<T, S>*) arc->getTargetNode();
					if(ghost->getIncomingArcs().size() == 0
							&& ghost->getOutgoingArcs().size() == 0) {
						this->ghostNodes.erase(ghost->getId());
						delete ghost;
					}
					this->ghostArcs.erase(arc->getId());
					delete (GhostArc<T, S>*) arc;
				}
			}

			/**
			 * Deletes ghost nodes and arcs remaining in this graph.
			 */
			template<class T, template<typename> class S> GhostGraph<T, S>::~GhostGraph() {
				for(auto node : this->ghostNodes) {
					delete node.second;
				}
				for(auto arc : this->ghostArcs) {
					delete arc.second;
				}
			}

		}
	}
}
#endif
