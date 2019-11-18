#include "graph.h"
#include "zoltan_cpp.h"

namespace FPMAS {
	namespace graph {
		template<class T> int zoltan_num_obj(void *data, int* ierr);
		template<class T> void zoltan_obj_list(
				void *, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int, float *, int *
				);
		template<class T> void zoltan_num_edges_multi_fn(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
				);
		template<class T> void zoltan_edge_list_multi_fn(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, ZOLTAN_ID_PTR, int *, int, float *, int *
				);


		template<class T> class DistributedGraph : public Graph<T> {
			private:
				Zoltan* zoltan;

			public:
				DistributedGraph<T>(Zoltan*);

				void distribute();

		};

		template<class T> DistributedGraph<T>::DistributedGraph(Zoltan* z)
			: Graph<T>(), zoltan(z) {
				zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan_num_obj<T>, this);
				zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan_obj_list<T>, this);
				zoltan->Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan_num_edges_multi_fn<T>, this);
				zoltan->Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan_edge_list_multi_fn<T>, this);

			}

		/**
		 * Returns the number of nodes currently managed by the current processor
		 * (i.e. the number of nodes contained in the local DistributedGraph
		 * instance).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_OBJ_FN).
		 *
		 * @param data user data (local DistributedGraph instance)
		 * @param ierr Result : error code
		 * @return numbe of nodes managed by the current process
		 */
		template<class T> int zoltan_num_obj(void *data, int* ierr) {
			return ((DistributedGraph<T>*) data)->getNodes().size();
		}

		/**
		 * Convenient function to rebuild a regular node id, as an
		 * unsigned long, from a ZOLTAN_ID_PTR global id array, that actually
		 * stores 2 unsigned int for each node unsigned long id.
		 * So, with our configuration, we use 2 unsigned int in Zoltan to
		 * represent each id. The `i` input parameter should correspond to the
		 * first part index of the node to query in the `global_ids` array. In
		 * consequence, `i` should actually always be even.
		 *
		 * @param global_ids a Zoltan global ids array
		 * @param i index of the first id part of the node id
		 * @return rebuilt node id
		 */
		// Note : Zoltan should be able to directly use unsigned long as ids.
		// However, this require to set up compile time flags to Zoltan, so we
		// use 2 entries of the default zoltan unsigned int instead, to make it
		// compatible with any Zoltan installation.
		// Passing compile flags to the custom embedded CMake Zoltan
		// installation might also be possible.
		unsigned long node_id(const ZOLTAN_ID_PTR global_ids) {
			return ((unsigned long) (global_ids[0] << 16)) | global_ids[1];
		}

		void write_zoltan_node_id(unsigned long node_id, ZOLTAN_ID_PTR global_ids) {
			global_ids[0] = (node_id & 0xFFFF0000) >> 16 ;
			global_ids[1] = (node_id & 0xFFFF);
		}

		/**
		 * Lists node ids contained on the current process (i.e. in the
		 * DistributedGraph instance associated to the process).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_OBJ_LIST_FN).
		 *
		 * @param data user data (local DistributedGraph instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param global_ids Result : global ids assigned to processor
		 * @param local_ids Result : local ids assigned to processor (unused)
		 * @param wgt_dim Number of weights of each object, defined by OBJ_WEIGHT_DIM (should be 1)
		 * @param obj_wgts Result : weights list
		 * @param ierr Result : error code
		 */
		template<class T> void zoltan_obj_list(
				void *data,
				int num_gid_entries, 
				int num_lid_entries,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int wgt_dim,
				float *obj_wgts,
				int *ierr
				) {
			int i = 0;
			for(auto n : ((DistributedGraph<T>*) data)->getNodes()) {
				Node<T>* node = n.second;


				write_zoltan_node_id(node->getId(), &global_ids[2*i]);

				obj_wgts[i++] = node->getWeight();
			}
		}

		/**
		 * Counts the number of outgoing arcs of each node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraph instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param num_obj number of objects IDs in global_ids
		 * @param global_ids Global IDs of object whose number of edges should be returned
		 * @param local_ids Same for local ids, unused
		 * @param num_edges Result : number of outgoing arc for each node
		 * @param ierr Result : error code
		 */
		template<class T> void zoltan_num_edges_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				int *ierr
				) {
			std::unordered_map<unsigned long, Node<T>*> nodes = ((DistributedGraph<T>*) data)->getNodes();
			for(int i = 0; i < num_obj; i++) {
				Node<T>* node = nodes[node_id(&global_ids[2*i])];
				num_edges[i] = node->getOutgoingArcs().size();
			}
		}

		/**
		 * List node IDs connected to each node through outgoing arcs for each
		 * node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraph instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param num_obj number of objects IDs in global_ids
		 * @param global_ids Global IDs of object whose list of edges should be returned
		 * @param local_ids Same for local ids, unused
		 * @param num_edges number of edges of each corresponding node
		 * @param nbor_global_id Result : neighbor ids for each node
		 * @param nbor_procs Result : processor identifier of each neighbor in
		 * nbor_global_id
		 * @param wgt_dim number of edge weights
		 * @param ewgts Result : edge weight for each neighbor
		 * @param ierr Result : error code
		 */
		template<class T> void zoltan_edge_list_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				ZOLTAN_ID_PTR nbor_global_id,
				int *nbor_procs,
				int wgt_dim,
				float *ewgts,
				int *ierr) {

			std::unordered_map<unsigned long, Node<T>*> nodes = ((DistributedGraph<T>*) data)->getNodes();

			int neighbor_index = 0;
			for (int i = 0; i < num_obj; ++i) {
				Node<T>* node = nodes[node_id(&global_ids[num_gid_entries * i])];
				for(int j = 0; j < node->getOutgoingArcs().size(); j++) {
					Arc<T>* arc = node->getOutgoingArcs().at(j);
					write_zoltan_node_id(arc->getTargetNode()->getId(), &nbor_global_id[neighbor_index * num_gid_entries]);
					
					// Temporary
					MPI_Comm_rank(MPI_COMM_WORLD, &nbor_procs[neighbor_index]);

					ewgts[neighbor_index] = 1.;
					neighbor_index++;
				}
			}

		}



	}
}

