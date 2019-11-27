#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "zoltan/zoltan_lb.h"
#include "zoltan/zoltan_utils.h"
#include "zoltan/zoltan_node_migrate.h"
#include "zoltan/zoltan_arc_migrate.h"

#include "utils/config.h"
#include "communication/communication.h"

#include "proxy.h"

using FPMAS::communication::MpiCommunicator;

namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;
		template<class T> class GhostArc;

		namespace zoltan {
			template<class T> int num_obj(void *data, int* ierr);
			template<class T> void obj_list(
					void *, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int, float *, int *
					);
			template<class T> void num_edges_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
					);
			template<class T> void edge_list_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, ZOLTAN_ID_PTR, int *, int, float *, int *
					);
			namespace node {
				template<class T> void obj_size_multi_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
						); 

				template<class T> void pack_obj_multi_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
						);

				template<class T> void unpack_obj_multi_fn(
						void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
						);
				template<class T> void post_migrate_pp_fn(
						void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *,
						int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *
						);
			}
		}

		/**
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors using Zoltan.
		 */
		template<class T> class DistributedGraph : public Graph<T> {
			friend GhostArc<T>;
			friend void zoltan::node::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::arc::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::node::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
			friend void zoltan::arc::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *);
			friend void zoltan::node::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			friend void zoltan::arc::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			friend void zoltan::node::post_migrate_pp_fn<T>(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *,
      				int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *
					);


			private:
				MpiCommunicator mpiCommunicator;
				Zoltan* zoltan;
				graph::proxy::Proxy proxy;

				void setZoltanNodeMigration();
				void setZoltanArcMigration();
				std::unordered_map<unsigned long, std::string> node_serialization_cache;
				std::unordered_map<unsigned long, std::string> arc_serialization_cache;

				/*
				 * Zoltan structures used to manage nodes and arcs migration
				 */
				// Node export buffer
				int* export_node_procs;

				// Arc migration buffers
				int export_arcs_num;
				ZOLTAN_ID_PTR export_arcs_global_ids;
				int* export_arcs_procs;

				std::unordered_map<unsigned long, GhostNode<T>*> ghostNodes;
				std::unordered_map<unsigned long, GhostArc<T>*> ghostArcs;
				void linkGhostNode(GhostNode<T>*, Node<T>*, unsigned long);
				void linkGhostNode(Node<T>*, GhostNode<T>*, unsigned long);
				void linkGhostNode(GhostNode<T>*, GhostNode<T>*, unsigned long);
				

			public:
				DistributedGraph<T>();
				MpiCommunicator getMpiCommunicator() const;

				void distribute();
				void synchronize();

				GhostNode<T>* buildGhostNode(Node<T> node);

				std::unordered_map<unsigned long, GhostNode<T>*> getGhostNodes();
				std::unordered_map<unsigned long, GhostArc<T>*> getGhostArcs();

				~DistributedGraph<T>();

		};

		/**
		 * DistributedGraph constructor.
		 *
		 * Instanciates and configure a new Zoltan instance accross all the
		 * available cores.
		 */
		template<class T> DistributedGraph<T>::DistributedGraph() {
			proxy.setLocalProc(mpiCommunicator.getRank());

			this->zoltan = new Zoltan(mpiCommunicator.getMpiComm());
			FPMAS::config::zoltan_config(this->zoltan);

			// Initializes Zoltan Node Load Balancing functions
			this->zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan::num_obj<T>, this);
			this->zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan::obj_list<T>, this);
			this->zoltan->Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan::num_edges_multi_fn<T>, this);
			this->zoltan->Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan::edge_list_multi_fn<T>, this);

			// Initializes Zoltan Arc migration buffers
			this->export_arcs_global_ids = (ZOLTAN_ID_PTR) std::malloc(0);
			this->export_arcs_procs = (int*) std::malloc(0);
		}

		/**
		 * Returns the MpiCommunicator used by the Zoltan instance of this
		 * DistrDistributedGraph.
		 *
		 * @return mpiCommunicator associated to this graph
		 */
		template<class T> MpiCommunicator DistributedGraph<T>::getMpiCommunicator() const {
			return this->mpiCommunicator;
		}

		/**
		 * Configures Zoltan to migrate nodes.
		 */
		template<class T> void DistributedGraph<T>::setZoltanNodeMigration() {
			zoltan->Set_Obj_Size_Multi_Fn(zoltan::node::obj_size_multi_fn<T>, this);
			zoltan->Set_Pack_Obj_Multi_Fn(zoltan::node::pack_obj_multi_fn<T>, this);
			zoltan->Set_Unpack_Obj_Multi_Fn(zoltan::node::unpack_obj_multi_fn<T>, this);
			zoltan->Set_Post_Migrate_PP_Fn(zoltan::node::post_migrate_pp_fn<T>, this);
		}

		/**
		 * Configures Zoltan to migrate arcs.
		 */
		template<class T> void DistributedGraph<T>::setZoltanArcMigration() {
			zoltan->Set_Obj_Size_Multi_Fn(zoltan::arc::obj_size_multi_fn<T>, this);
			zoltan->Set_Pack_Obj_Multi_Fn(zoltan::arc::pack_obj_multi_fn<T>, this);
			zoltan->Set_Unpack_Obj_Multi_Fn(zoltan::arc::unpack_obj_multi_fn<T>, this);
		}


		/**
		 * Distributes the graph accross the available cores using Zoltan.
		 *
		 * The graph is distributed using
		 * [Zoltan::LB_Partition](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_lb.html#Zoltan_LB_Partition)
		 * and
		 * [Zoltan::Migrate](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_mig.html#Zoltan_Migrate).
		 * Query functions used are defined in the FPMAS::graph::zoltan namespace.
		 *
		 * Because of the Zoltan behavior, the current process will block until
		 * all other involved process also call the distribute() function.
		 *
		 */
		// TODO: describe the full migration process
		template<class T> void DistributedGraph<T>::distribute() {
			int changes;
      		int num_gid_entries;
			int num_lid_entries;
			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int* export_to_part;

			int * import_procs;
			int * import_to_part;
			int num_export;
			ZOLTAN_ID_PTR export_global_ids;
			ZOLTAN_ID_PTR export_local_ids;
			
			// Computes Zoltan partitioning
			this->zoltan->LB_Partition(
				changes,
				num_gid_entries,
				num_lid_entries,
				num_import,
				import_global_ids,
				import_local_ids,
				import_procs,
				import_to_part,
				num_export,
				export_global_ids,
				export_local_ids,
				this->export_node_procs,
				export_to_part
				);

			if(changes > 0) {
				// Prepares Zoltan to migrate nodes
				this->setZoltanNodeMigration();

				// Migrate nodes from the load balancing
				// Arcs to export are computed in the post_migrate_pp_fn step.
				this->zoltan->Migrate(
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					num_export,
					export_global_ids,
					export_local_ids,
					this->export_node_procs,
					export_to_part
					);

				// Prepares Zoltan to migrate arcs associated to the exported
				// nodes.
				this->setZoltanArcMigration();

				// Unused buffer
				unsigned int export_arcs_local_ids[0];

				// Arcs to import
				int import_arcs_num;
				ZOLTAN_ID_PTR import_arcs_global_ids;
				ZOLTAN_ID_PTR import_arcs_local_ids;
				int* import_arcs_procs;
				int* import_arcs_parts;

				// Computes import/export arcs list from each local arc exports
				// lists, computing at node export.
				this->zoltan->Invert_Lists(
					this->export_arcs_num,
					this->export_arcs_global_ids,
					export_arcs_local_ids,
					this->export_arcs_procs,
					this->export_arcs_procs, // parts = procs
					import_arcs_num,
					import_arcs_global_ids,
					import_arcs_local_ids,
					import_arcs_procs,
					import_arcs_parts
					);

				// Performs arcs migration
				this->zoltan->Migrate(
					import_arcs_num,
					import_arcs_global_ids,
					import_arcs_local_ids,
					import_arcs_procs,
					import_arcs_parts,
					this->export_arcs_num,
					this->export_arcs_global_ids,
					export_arcs_local_ids,
					this->export_arcs_procs,
					this->export_arcs_procs // parts = procs
					);

				// The next steps will remove exported nodes from the local
				// graph, creating corresponding ghost nodes when necessary

				// Computes the set of ids of exported nodes
				std::set<unsigned long> exportedNodeIds;
				for(int i = 0; i < num_export; i++) {
					exportedNodeIds.insert(zoltan::utils::read_zoltan_id(&export_global_ids[i * num_gid_entries]));
				}

				// Computes which ghost nodes must be created.
				// For each exported node, a ghost node is created if and only
				// if at least one local node is still connected to the
				// exported node.
				std::vector<Node<T>*> ghostNodesToBuild;
				for(auto id : exportedNodeIds) {
					Node<T>* node = this->getNode(id);
					bool buildGhost = false;
					for(auto arc : node->getOutgoingArcs()) {
						if(exportedNodeIds.count(arc->getTargetNode()->getId()) == 0) {
							buildGhost = true;
							break;
						}
					}
					if(!buildGhost) {
						for(auto arc : node->getIncomingArcs()) {
							if(exportedNodeIds.count(arc->getSourceNode()->getId()) == 0) {
								buildGhost = true;
								break;
							}
						}
					}
					if(buildGhost) {
						ghostNodesToBuild.push_back(this->getNode(id));
					}
				}
				// Builds the ghost nodes
				for(auto node : ghostNodesToBuild) {
					this->buildGhostNode(*node);
				}

				// Remove nodes and collect fossils
				Fossil<T> ghostFossils;
				for(auto id : exportedNodeIds) {
					ghostFossils.merge(this->removeNode(id));
				}

				// TODO : schema with fossils example
				// For info, see the Fossil and removeNode docs
				for(auto arc : ghostFossils.arcs) {
					this->ghostArcs.erase(arc->getId());
					delete arc;
				}
			}

			// Frees the zoltan buffers
			this->zoltan->LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan->LB_Free_Part(
					&export_global_ids,
					&export_local_ids,
					&this->export_node_procs,
					&export_to_part
					);
		}

		template<class T> void DistributedGraph<T>::synchronize() {
			int import_ghosts_num = this->ghostNodes.size(); 
			ZOLTAN_ID_PTR import_ghosts_global_ids = (ZOLTAN_ID_PTR) std::malloc(sizeof(unsigned int) * import_ghosts_num * 2);
			ZOLTAN_ID_PTR import_ghosts_local_ids;
			int* import_ghost_procs = (int*) std::malloc(sizeof(int) * import_ghosts_num);
			int* import_ghost_parts = (int*) std::malloc(sizeof(int) * import_ghosts_num);

		}

		/**
		 * Builds a GhostNode as an exact copy of the specified node.
		 *
		 * For each incoming/outgoing arcs of the input nodes, the
		 * corresponding source/target nodes are linked to the built GhostNode
		 * with GhostArc s. In other terms, all the arcs linked to the original
		 * node are duplicated.
		 *
		 * The original node can then be safely removed from the graph.
		 *
		 * @param node node from which a ghost copy must be created
		 * @param node_proc proc where the real node lives
		 */
		template<class T> GhostNode<T>* DistributedGraph<T>::buildGhostNode(Node<T> node) {
			// Copy the gNode from the original node, including arcs data
			GhostNode<T>* gNode = new GhostNode<T>(node);
			// Register the new GhostNode
			this->ghostNodes[gNode->getId()] = gNode;

			// Replaces the incomingArcs list by proper GhostArcs
			std::vector<Arc<T>*> temp_in_arcs = gNode->incomingArcs;
			gNode->incomingArcs.clear();	
			for(auto arc : temp_in_arcs) {
				Node<T>* localSourceNode = arc->getSourceNode();
				this->linkGhostNode(localSourceNode, gNode, arc->getId());
			}

			// Replaces the outgoingArcs list by proper GhostArcs
			std::vector<Arc<T>*> temp_out_arcs = gNode->outgoingArcs;
			gNode->outgoingArcs.clear();
			for(auto arc : temp_out_arcs) {
				Node<T>* localTargetNode = arc->getTargetNode();
				this->linkGhostNode(gNode, localTargetNode, arc->getId());
			}

			return gNode;

		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void DistributedGraph<T>::linkGhostNode(GhostNode<T>* source, Node<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void DistributedGraph<T>::linkGhostNode(Node<T>* source, GhostNode<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Links the specified nodes with a GhostArc.
		 */ 
		template<class T> void DistributedGraph<T>::linkGhostNode(GhostNode<T>* source, GhostNode<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		/**
		 * Returns the map of GhostNode s currently living in this graph.
		 *
		 * @return ghost nodes
		 */
		template<class T> std::unordered_map<unsigned long, GhostNode<T>*> DistributedGraph<T>::getGhostNodes() {
			return this->ghostNodes;
		}

		/**
		 * Returns the map of GhostArc s currently living in this graph.
		 *
		 * @return ghost arcs
		 */
		template<class T> std::unordered_map<unsigned long, GhostArc<T>*> DistributedGraph<T>::getGhostArcs() {
			return this->ghostArcs;
		}

		/**
		 * DistributedGraph destructor.
		 *
		 * `deletes` ghost nodes and arcs, and data associated to Zoltan.
		 */
		template<class T> DistributedGraph<T>::~DistributedGraph() {
			for(auto node : this->ghostNodes) {
				delete node.second;
			}
			for(auto arc : this->ghostArcs) {
				delete arc.second;
			}
			std::free(this->export_arcs_global_ids);
			std::free(this->export_arcs_procs);
			delete this->zoltan;
		}


	}
}
#endif
