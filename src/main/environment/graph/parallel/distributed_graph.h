#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "../base/graph.h"
#include "zoltan_cpp.h"
#include "olz.h"
#include "zoltan_fn.h"


namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;
		template<class T> class GhostArc;

		/**
		 * The FPMAS::graph::zoltan namespace contains definitions of all the
		 * required Zoltan query functions to compute and distribute graph
		 * partitions.
		 */
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
			template<class T> void obj_size_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
					); 

			template<class T> void pack_obj_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
					);

			template<class T> void unpack_obj_multi_fn(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
			template<class T> void mid_migrate_pp_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR , int *,
      				int *, int , ZOLTAN_ID_PTR , ZOLTAN_ID_PTR , int *, int *,int *
					);
		}

		template<class T> class DistributedGraph : public Graph<T> {
			friend GhostArc<T>;
			friend void zoltan::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
				);
			friend void zoltan::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);

			private:
				Zoltan* zoltan;
				std::unordered_map<unsigned long, std::string> node_serialization_cache;

				std::unordered_map<unsigned long, GhostNode<T>*> ghostNodes;
				std::unordered_map<unsigned long, GhostArc<T>*> ghostArcs;
				void linkGhostNode(Node<T>*, Node<T>*, unsigned long);
				

			public:
				DistributedGraph<T>(Zoltan*);

				int distribute();

				GhostNode<T>* buildGhostNode(Node<T> node, int);

				~DistributedGraph<T>();

		};

		/**
		 * DistributedGraph constructor.
		 *
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors through the provided Zoltan
		 * instance.
		 *
		 * @param z pointer to an initiliazed zoltan instance
		 */
		template<class T> DistributedGraph<T>::DistributedGraph(Zoltan* z)
			: Graph<T>(), zoltan(z) {
				zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan::num_obj<T>, this);
				zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan::obj_list<T>, this);
				zoltan->Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan::num_edges_multi_fn<T>, this);
				zoltan->Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan::edge_list_multi_fn<T>, this);

				zoltan->Set_Obj_Size_Multi_Fn(FPMAS::graph::zoltan::obj_size_multi_fn<T>, this);
				zoltan->Set_Pack_Obj_Multi_Fn(FPMAS::graph::zoltan::pack_obj_multi_fn<T>, this);
				zoltan->Set_Unpack_Obj_Multi_Fn(FPMAS::graph::zoltan::unpack_obj_multi_fn<T>, this);
				zoltan->Set_Mid_Migrate_PP_Fn(FPMAS::graph::zoltan::mid_migrate_pp_fn<T>, this);

			}

		/**
		 * Distributes the graph accross the processors using Zoltan.
		 *
		 * Nodes are distributed among processors belonging to the MPI
		 * communicator used to instanciate the zoltan instance associated to
		 * this distributed graph.
		 *
		 * The graph is distributed using
		 * [Zoltan::LB_Partition](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_lb.html#Zoltan_LB_Partition)
		 * and
		 * [Zoltan::Migrate](https://cs.sandia.gov/Zoltan/ug_html/ug_interface_mig.html#Zoltan_Migrate). Query functions used are defined in the FPMAS::graph::zoltan namespace.
		 *
		 * Because of the Zoltan behavior, the current process will block until
		 * all other involved process also call the distribute() function.
		 *
		 * @return Zoltan migrate [error
		 * code](https://cs.sandia.gov/Zoltan/ug_html/ug_interface.html#Error%20Codes)
		 */
		template<class T> int DistributedGraph<T>::distribute() {
			int changes;
      		int num_gid_entries;
			int num_lid_entries;
			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int * import_procs;
			int * import_to_part;
			int num_export;
			ZOLTAN_ID_PTR export_global_ids;
			ZOLTAN_ID_PTR export_local_ids;
			int * export_procs;
			int * export_to_part;
			
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
				export_procs,
				export_to_part
				);

			int result = ZOLTAN_OK;

			if(changes > 0)
				result = this->zoltan->Migrate(
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					num_export,
					export_global_ids,
					export_local_ids,
					export_procs,
					export_to_part
					);

			this->zoltan->LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan->LB_Free_Part(
					&export_global_ids,
					&export_local_ids,
					&export_procs,
					&export_to_part
					);

			return result;

		}

		template<class T> GhostNode<T>* DistributedGraph<T>::buildGhostNode(Node<T> node, int node_proc) {
			// Copy the gNode from the original node, including arcs data
			GhostNode<T>* gNode = new GhostNode<T>(node, node_proc);
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

		template<class T> void DistributedGraph<T>::linkGhostNode(Node<T>* source, Node<T>* target, unsigned long arc_id) {
			this->ghostArcs[arc_id] =
				new GhostArc<T>(arc_id, source, target);
		}

		template<class T> DistributedGraph<T>::~DistributedGraph() {
			for(auto node : this->ghostNodes) {
				delete node.second;
			}
			for(auto arc : this->ghostArcs) {
				delete arc.second;
			}
		}


	}
}
#endif
