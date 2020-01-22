#ifndef ZOLTAN_GHOST_NODE_MIGRATE_H
#define ZOLTAN_GHOST_NODE_MIGRATE_H

#include "zoltan_cpp.h"

#include "zoltan_utils.h"
#include "../olz.h"
#include "../synchro/sync_data.h"


namespace FPMAS {
	namespace graph {

		template<class T, template<typename> class S> class DistributedGraph;

		using synchro::SyncData;
		using synchro::LocalData;
		using synchro::GhostData;

		using synchro::GhostNode;

		namespace zoltan {
			using utils::write_zoltan_id;
			using utils::read_zoltan_id;
			/**
			 * The zoltan::ghost namespace defines functions used to migrate
			 * ghost nodes data using zoltan.
			 */
			namespace ghost {
				/**
				 * Computes the buffer sizes required to serialize ghost node data corresponding
				 * to node global_ids.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_OBJ_SIZE_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_lid_entries number of entries used to describe local ids (should be 0)
				 * @param num_ids number of ghost node data to serialize
				 * @param global_ids global ids of nodes to serialize
				 * @param local_ids unused
				 * @param sizes Result : buffer sizes for each node
				 * @param ierr Result : error code
				 */
				template<class T, template<typename> class S = GhostData> void obj_size_multi_fn(
						void *data,
						int num_gid_entries,
						int num_lid_entries,
						int num_ids,
						ZOLTAN_ID_PTR global_ids,
						ZOLTAN_ID_PTR local_ids,
						int *sizes,
						int *ierr) {


					DistributedGraph<T, S>* graph = (DistributedGraph<T, S>*) data;
					std::unordered_map<unsigned long, Node<SyncData<T>>*> nodes = graph->getNodes();
					for (int i = 0; i < num_ids; i++) {
						Node<SyncData<T>>* node = nodes.at(read_zoltan_id(&global_ids[i * num_gid_entries]));

						if(graph->getGhost().ghost_node_serialization_cache.count(node->getId()) == 1) {
							sizes[i] = graph->getGhost().ghost_node_serialization_cache.at(node->getId()).size()+1;
						}
						else {
							json json_ghost_node = *node;

							std::string serial_node = json_ghost_node.dump();

							sizes[i] = serial_node.size() + 1;

							graph->getGhost().ghost_node_serialization_cache[node->getId()] = serial_node;
						}
					}

				}

				/**
				 * Serializes the ghost node data corresponding to input list of nodes as a json string.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_PACK_OBJ_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_lid_entries number of entries used to describe local ids (should be 0)
				 * @param num_ids number of nodes to pack
				 * @param global_ids global ids of ghost node data to pack
				 * @param local_ids unused
				 * @param dest destination part numbers
				 * @param sizes buffer sizes for each object
				 * @param idx each object starting point in buf
				 * @param buf communication buffer
				 * @param ierr Result : error code
				 */
				template<class T, template<typename> class S = GhostData> void pack_obj_multi_fn(
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

					DistributedGraph<T, S>* graph = (DistributedGraph<T, S>*) data;
					// The node should actually be serialized when computing
					// the required buffer size. For efficiency purpose, we temporarily
					// store the result and delete it when it is packed.
					std::unordered_map<unsigned long, std::string>* serial_cache
						= &graph->getGhost().ghost_node_serialization_cache;
					for (int i = 0; i < num_ids; ++i) {
						// Rebuilt node id
						unsigned long id = read_zoltan_id(&global_ids[i * num_gid_entries]);

						// Retrieves the serialized node
						std::string node_str = serial_cache->at(id);

						// Copy str to zoltan buffer
						std::sprintf(&buf[idx[i]], "%s", node_str.c_str());
					}
					// Everything has been sent, clears serialization cache
					serial_cache->clear();
				}

				/**
				 * Deserializes received ghost nodes data and update
				 * corresponding ghost nodes.
				 *
				 * For more information about this function, see the [Zoltan
				 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_UNPACK_OBJ_MULTI_FN).
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_ids number of nodes to unpack
				 * @param global_ids item global ids
				 * @param sizes buffer sizes for each object
				 * @param idx each object starting point in buf
				 * @param buf communication buffer
				 * @param ierr Result : error code
				 *
				 */
				template<class T, template<typename> class S = GhostData> void unpack_obj_multi_fn(
						void *data,
						int num_gid_entries,
						int num_ids,
						ZOLTAN_ID_PTR global_ids,
						int *sizes,
						int *idx,
						char *buf,
						int *ierr) {

					DistributedGraph<T, S>* graph = (DistributedGraph<T, S>*) data;
					for (int i = 0; i < num_ids; ++i) {
						int node_id = read_zoltan_id(&global_ids[i * num_gid_entries]);
						json json_node = json::parse(&buf[idx[i]]);

						GhostNode<T, S>* ghost = graph->getGhost().getNodes().at(node_id);
						Node<LocalData<T>> node_update = json_node.get<Node<LocalData<T>>>();

						ghost->data().get() = node_update.data().get();
						ghost->setWeight(node_update.getWeight());
					}

				}


			}
		}
	}
}

#endif
