#ifndef ZOLTAN_ARC_MIGRATE_H
#define ZOLTAN_ARC_MIGRATE_H

#include "zoltan_cpp.h"

#include "../distributed_graph.h"

using FPMAS::graph::Arc;
using FPMAS::graph::DistributedGraph;

namespace FPMAS {
	namespace graph {
		namespace zoltan {
			namespace arc {
				/**
				 * Computes the buffer sizes required to serialize nodes corresponding
				 * to global_ids.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_OBJ_SIZE_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_lid_entries number of entries used to describe local ids (should be 0)
				 * @param num_ids number of nodes to serialize
				 * @param global_ids global ids of nodes to serialize
				 * @param local_ids unused
				 * @param sizes Result : buffer sizes for each node
				 * @param ierr Result : error code
				 */
				template<class T> void obj_size_multi_fn(
						void *data,
						int num_gid_entries,
						int num_lid_entries,
						int num_ids,
						ZOLTAN_ID_PTR global_ids,
						ZOLTAN_ID_PTR local_ids,
						int *sizes,
						int *ierr) {


					std::unordered_map<unsigned long, Arc<T>*> arcs = ((DistributedGraph<T>*) data)->getArcs();
					for (int i = 0; i < num_ids; i++) {
						Arc<T>* arc = arcs.at(read_zoltan_id(&global_ids[i * num_gid_entries]));

						json json_node = *arc;
						std::string serial_node = json_node.dump();

						sizes[i] = serial_node.size() + 1;

						((DistributedGraph<T>*) data)->arc_serialization_cache[arc->getId()] = serial_node;

					}

				}

				/**
				 * Serializes the input list of nodes as a json string.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_PACK_OBJ_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_lid_entries number of entries used to describe local ids (should be 0)
				 * @param num_ids number of nodes to pack
				 * @param global_ids global ids of nodes to pack
				 * @param local_ids unused
				 * @param dest destination part numbers
				 * @param sizes buffer sizes for each object
				 * @param idx each object starting point in buf
				 * @param buf communication buffer
				 * @param ierr Result : error code
				 */
				template<class T> void pack_obj_multi_fn(
						void *data,
						int num_gid_entries,
						int num_lid_entries,
						int num_ids,
						ZOLTAN_ID_PTR global_ids,
						ZOLTAN_ID_PTR local_ids,
						int *dest,
						int *sizes,
						int *idx,
						char *buf,
						int *ierr) {

					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					// The node should actually be serialized when computing
					// the required buffer size. For efficiency purpose, we temporarily
					// store the result and delete it when it is packed.
					std::unordered_map<unsigned long, std::string> serial_cache
						= graph->arc_serialization_cache;
					for (int i = 0; i < num_ids; ++i) {
						// Rebuilt node id
						unsigned long id = read_zoltan_id(&global_ids[i * num_gid_entries]);

						// Retrieves the serialized node
						std::string arc_str = serial_cache.at(id);
						for(int j = 0; j < sizes[i] - 1; j++) {
							buf[idx[i] + j] = arc_str[j];
						}
						buf[idx[i] + sizes[i] - 1] = 0; // str final char

						// Removes entry from the serialization buffer
						serial_cache.erase(id);
					}

				}

				/**
				 * Deserializes received nodes to the local distributed graph.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_UNPACK_OBJ_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_ids number of nodes to pack
				 * @param global_ids item global ids
				 * @param sizes buffer sizes for each object
				 * @param idx each object starting point in buf
				 * @param buf communication buffer
				 * @param ierr Result : error code
				 *
				 */
				template<class T> void unpack_obj_multi_fn(
						void *data,
						int num_gid_entries,
						int num_ids,
						ZOLTAN_ID_PTR global_ids,
						int *sizes,
						int *idx,
						char *buf,
						int *ierr) {

					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					for (int i = 0; i < num_ids; ++i) {
						read_zoltan_id(&global_ids[i * num_gid_entries]);
						json json_node = json::parse(&buf[idx[i]]);

						// Json is unserialized in a temporary arc, with "fake"
						// nodes that just contains ID. We don't know yet which
						// nodes are on this local process or not.
						Arc<T> tempArc = json_node.get<Arc<T>>();
						if(
							graph->getNodes().count(
									tempArc.getSourceNode()->getId()
									) > 0
								&&
							graph->getNodes().count(
									tempArc.getTargetNode()->getId()
									) > 0
						  ) {
							// Imported arc is completely local.
							// We can built it locally. Fake nodes are deleted
							// properly by the graph::link(Arc<T> tempArc).
							((DistributedGraph<T>*) data)->link(tempArc);
						}
						else {
							// TODO: Temporarily save ghost arcs, build ghost
							// node requests

						}
					}

				}

			}
		}
	}
}

#endif
