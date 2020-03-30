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

	template<typename T, SYNC_MODE> class DistributedGraphBase;
	template<typename T, SYNC_MODE> class DistributedGraph;

	using synchro::wrappers::SyncData;

	/**
	 * The FPMAS::graph::zoltan namespace contains definitions of all the
	 * required Zoltan query functions to compute load balancing partitions
	 * based on the current graph nodes.
	 */
	namespace zoltan {

		/**
		 * Returns the number of nodes currently managed by the current processor
		 * (i.e. the number of nodes contained in the local DistributedGraphBase
		 * instance).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_OBJ_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param ierr Result : error code
		 * @return numbe of nodes managed by the current process
		 */
		template<typename T, SYNC_MODE> int num_obj(void *data, int* ierr) {
			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			return graph->toBalance.size();
		}

		/**
		 * Lists node ids contained on the current process (i.e. in the
		 * DistributedGraphBase instance associated to the process).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_OBJ_LIST_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param global_ids Result : global ids assigned to processor
		 * @param local_ids Result : local ids assigned to processor (unused)
		 * @param wgt_dim Number of weights of each object, defined by OBJ_WEIGHT_DIM (should be 1)
		 * @param obj_wgts Result : weights list
		 * @param ierr Result : error code
		 */
		template<typename T, SYNC_MODE> void obj_list(
				void *data,
				int num_gid_entries, 
				int num_lid_entries,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int wgt_dim,
				float *obj_wgts,
				int *ierr
				) {
			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			int i = 0;
			for(auto n : graph->toBalance) {
				utils::write_zoltan_id(n->getId(), &global_ids[i * num_gid_entries]);
				obj_wgts[i++] = n->getWeight();
			}
		}

		/**
		 * Counts the number of outgoing arcs of each node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
		 * @param num_gid_entries number of entries used to describe global ids (should be 2)
		 * @param num_lid_entries number of entries used to describe local ids (should be 0)
		 * @param num_obj number of objects IDs in global_ids
		 * @param global_ids Global IDs of object whose number of edges should be returned
		 * @param local_ids Same for local ids, unused
		 * @param num_edges Result : number of outgoing arc for each node
		 * @param ierr Result : error code
		 */
		template<typename T, SYNC_MODE> void num_edges_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_obj,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *num_edges,
				int *ierr
				) {
			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			auto currentNodes = graph->toBalance;
			for(int i = 0; i < num_obj; i++) {
				auto node = graph->getNode(
						utils::read_zoltan_id(&global_ids[i * num_gid_entries])
						);
				int count = 0;
				for(auto arc : node->getOutgoingArcs()) {
					if(currentNodes.count(arc->getTargetNode()) > 0) {
						count++;
					}
				}
				num_edges[i] = count;
			}
		}

		/**
		 * List node IDs connected to each node through outgoing arcs for each
		 * node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (local DistributedGraphBase instance)
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
		template<typename T, SYNC_MODE> void edge_list_multi_fn(
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

			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			auto currentNodes = graph->toBalance;

			int neighbor_index = 0;
			for (int i = 0; i < num_obj; ++i) {
				auto node = graph->getNode(
						utils::read_zoltan_id(&global_ids[num_gid_entries * i])
						);
				for(auto arc : node->getOutgoingArcs()) {
					if(currentNodes.count(arc->getTargetNode()) > 0) {
						DistributedId targetId = arc->getTargetNode()->getId(); 
						utils::write_zoltan_id(targetId, &nbor_global_id[neighbor_index * num_gid_entries]);

						nbor_procs[neighbor_index] = graph->getProxy().getCurrentLocation(targetId);

						ewgts[neighbor_index] = 1.;
						neighbor_index++;
					}
				}
			}
		}

		template<typename T, SYNC_MODE> int num_fixed_obj_fn(
				void* data, int* ierr
				) {
			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			return graph->fixedVertices.size();
		}

		template<typename T, SYNC_MODE> void fixed_obj_list_fn(
			void *data,
			int num_fixed_obj,
			int num_gid_entries,
			ZOLTAN_ID_PTR fixed_gids,
			int *fixed_parts,
			int *ierr
		) {
			DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
			int i = 0;
			for(auto fixedVertice : graph->fixedVertices) {
				utils::write_zoltan_id(fixedVertice.first, &fixed_gids[i * num_gid_entries]);
				fixed_parts[i] = fixedVertice.second;
				i++;
			}
		}
	}
}
#endif
