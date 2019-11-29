#ifndef ZOLTAN_NODE_MIGRATE_H
#define ZOLTAN_NODE_MIGRATE_H

#include <set>

#include "zoltan_cpp.h"

#include "zoltan_utils.h"
#include "../distributed_graph.h"

using FPMAS::graph::zoltan::utils::write_zoltan_id;
using FPMAS::graph::zoltan::utils::read_zoltan_id;

using FPMAS::graph::Arc;
using FPMAS::graph::Node;
using FPMAS::graph::DistributedGraph;

namespace FPMAS {
	namespace graph {
		namespace zoltan {
			/**
			 * The zoltan::node namespace defines functions used to migrate
			 * nodes using zoltan.
			 */
			namespace node {
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


					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					std::unordered_map<unsigned long, Node<T>*> nodes = ((DistributedGraph<T>*) data)->getNodes();
					for (int i = 0; i < num_ids; i++) {
						Node<T>* node = nodes.at(read_zoltan_id(&global_ids[i * num_gid_entries]));

						if(graph->node_serialization_cache.count(node->getId()) == 1) {
							sizes[i] = graph->node_serialization_cache.at(node->getId()).size()+1;
						}
						else {
							json json_node = *node;

							json_node["origin"] = graph->proxy.getOrigin(node->getId());	

							std::string serial_node = json_node.dump();

							sizes[i] = serial_node.size() + 1;

							graph->node_serialization_cache[node->getId()] = serial_node;
						}
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
					std::unordered_map<unsigned long, std::string>* serial_cache
						= &graph->node_serialization_cache;
					for (int i = 0; i < num_ids; ++i) {
						// Rebuilt node id
						unsigned long id = read_zoltan_id(&global_ids[i * num_gid_entries]);

						// Retrieves the serialized node
						std::string node_str = serial_cache->at(id);
						for(int j = 0; j < node_str.size(); j++) {
							buf[idx[i] + j] = node_str[j];
						}
						buf[idx[i] + node_str.size()] = 0; // str final char
					}
					serial_cache->clear();

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
						json json_node = json::parse(&buf[idx[i]]);

						Node<T> node = json_node.get<Node<T>>();
						graph->buildNode(node);

						int origin = json_node.at("origin").get<int>();
						graph->proxy.setOrigin(node.getId(), origin);

					}

				}

				/**
				 * This function is called by Zoltan after node imports and
				 * exports have been performed.
				 *
				 * In our context, this functions computes arcs that need to be
				 * sent to each process according to exported nodes, and store
				 * those information in the proper DistributedGraph buffers.
				 *
				 * For each node, all incoming and outgoing arcs are 
				 * exported.
				 *
				 * @param data user data (local DistributedGraph instance)
				 * @param num_gid_entries number of entries used to describe global ids (should be 2)
				 * @param num_lid_entries number of entries used to describe local ids (should be 0)
				 * @param num_import number of nodes to import
				 * @param import_global_ids imported global ids
				 * @param import_local_ids imported local ids (unused)
				 * @param import_procs source processors ids
				 * @param import_to_part parts to which objects will be imported
				 * @param num_export number of nodes to export
				 * @param export_global_ids exported global ids
				 * @param export_local_ids exported local ids (unused)
				 * @param export_procs source processors ids
				 * @param export_to_part parts to which objects will be exported
				 * @param ierr Result : error code
				 */
				template<class T> void post_migrate_pp_fn(
						void *data,
						int num_gid_entries,
						int num_lid_entries,
						int num_import,
						ZOLTAN_ID_PTR import_global_ids,
						ZOLTAN_ID_PTR import_local_ids,
						int *import_procs,
						int *import_to_part,
						int num_export,
						ZOLTAN_ID_PTR export_global_ids,
						ZOLTAN_ID_PTR export_local_ids,
						int *export_procs,
						int *export_to_part,
						int *ierr) {

					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;

					std::unordered_map<unsigned long, Node<T>*> nodes = graph->getNodes();
					// Set used to ensure that each arc is sent at most once to
					// each process.
					std::set<std::pair<unsigned long, int>> exportedArcPairs;

					std::vector<Arc<T>*> arcsToExport;
					std::vector<int> procs;

					for (int i =0; i < num_export; i++) {
						unsigned long id = read_zoltan_id(&export_global_ids[i * num_gid_entries]);

						Node<T>* exported_node = nodes.at(id);
						int dest_proc = export_procs[i];

						// Updates Proxy
						graph->getProxy()->setCurrentLocation(id, dest_proc);

						// Computes arc exports
						for(auto arc : exported_node->getIncomingArcs()) {
							std::pair<unsigned long, int> arc_proc_pair =
								std::pair<unsigned long, int>(
										arc->getId(),
										export_procs[i]
										);

							if(exportedArcPairs.count(arc_proc_pair) == 0) {
								arcsToExport.push_back(arc);
								// Arcs will be sent to the proc and part associated to
								// the current node
								procs.push_back(export_procs[i]);

								exportedArcPairs.insert(arc_proc_pair);
							}
						}
						for(auto arc : exported_node->getOutgoingArcs()) {
							std::pair<unsigned long, int> arc_proc_pair =
								std::pair<unsigned long, int>(
										arc->getId(),
										export_procs[i]
										);
							if(exportedArcPairs.count(arc_proc_pair) == 0) {
								arcsToExport.push_back(arc);
								// Arcs will be sent to the proc and part associated to
								// the current node
								procs.push_back(export_procs[i]);

								exportedArcPairs.insert(arc_proc_pair);
							}
						}
					}
					graph->export_arcs_num = arcsToExport.size();

					graph->export_arcs_global_ids = (ZOLTAN_ID_PTR) std::malloc(sizeof(unsigned int) * graph->export_arcs_num * num_gid_entries);
					graph->export_arcs_procs = (int*) std::malloc(sizeof(int) * graph->export_arcs_num);

					for(int i = 0; i < graph->export_arcs_num; i++) {
						write_zoltan_id(arcsToExport.at(i)->getId(), &graph->export_arcs_global_ids[i * num_gid_entries]);
						graph->export_arcs_procs[i] = procs[i];
					}
				}
			}
		}
	}
}
#endif
