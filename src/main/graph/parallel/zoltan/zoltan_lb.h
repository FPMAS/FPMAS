#ifndef ZOLTAN_LB_H
#define ZOLTAN_LB_H

#include "zoltan_cpp.h"

#include "utils/macros.h"
#include "zoltan_utils.h"
#include "../synchro/sync_mode.h"
#include "../../base/node.h"

using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	using base::NodeId;
	using synchro::wrappers::SyncData;

	/**
	 * The FPMAS::graph::zoltan namespace contains definitions of all the
	 * required Zoltan query functions to compute load balancing partitions
	 * based on the current graph nodes.
	 */
	namespace zoltan {

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
		template<typename T, int N, SYNC_MODE> int num_obj(void *data, int* ierr) {
			return ((DistributedGraph<T, S, N>*) data)->getNodes().size();
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
		template<typename T, int N, SYNC_MODE> void obj_list(
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
			for(auto n : ((DistributedGraph<T, S, N>*) data)->getNodes()) {
				Node<std::unique_ptr<SyncData<T,N,S>>, N>* node = n.second;

				utils::write_zoltan_id(node->getId(), &global_ids[i * num_gid_entries]);

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
		template<typename T, int N, SYNC_MODE> void num_edges_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				int *ierr
				) {
			std::unordered_map<NodeId, Node<std::unique_ptr<SyncData<T,N,S>>, N>*> nodes
				= ((DistributedGraph<T, S, N>*) data)->getNodes();
			for(int i = 0; i < num_obj; i++) {
				Node<std::unique_ptr<SyncData<T,N,S>>, N>* node = nodes.at(
						utils::read_zoltan_id(&global_ids[i * num_gid_entries])
						);
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
		template<typename T, int N, SYNC_MODE> void edge_list_multi_fn(
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

			DistributedGraph<T, S, N>* graph = (DistributedGraph<T, S, N>*) data;
			std::unordered_map<NodeId, Node<std::unique_ptr<SyncData<T,N,S>>, N>*> nodes = graph->getNodes();

			int neighbor_index = 0;
			for (int i = 0; i < num_obj; ++i) {
				Node<std::unique_ptr<SyncData<T,N,S>>, N>* node = nodes.at(
						utils::read_zoltan_id(&global_ids[num_gid_entries * i])
						);
				for(int j = 0; j < node->getOutgoingArcs().size(); j++) {
					Arc<std::unique_ptr<SyncData<T,N,S>>, N>* arc = node->getOutgoingArcs().at(j);
					NodeId targetId = arc->getTargetNode()->getId(); 
					utils::write_zoltan_id(targetId, &nbor_global_id[neighbor_index * num_gid_entries]);

					// Temporary
					// MPI_Comm_rank(MPI_COMM_WORLD, &nbor_procs[neighbor_index]);
					nbor_procs[neighbor_index] = graph->getProxy().getCurrentLocation(targetId);

					ewgts[neighbor_index] = 1.;
					neighbor_index++;
				}
			}
		}
	}
}
#endif
