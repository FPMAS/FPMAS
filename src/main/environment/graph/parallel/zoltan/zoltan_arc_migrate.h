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


					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					std::unordered_map<unsigned long, Arc<T>*> arcs = graph->getArcs();
					for (int i = 0; i < num_ids; i++) {
						Arc<T>* arc = arcs.at(read_zoltan_id(&global_ids[i * num_gid_entries]));

						if(graph->arc_serialization_cache.count(arc->getId()) == 1) {
							sizes[i] = graph->arc_serialization_cache.at(arc->getId()).size() + 1;
						}
						else {
							json json_node = *arc;
							std::string serial_node = json_node.dump();

							sizes[i] = serial_node.size() + 1;

							graph->arc_serialization_cache[arc->getId()] = serial_node;
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
					}
					serial_cache.clear();
				}

				/**
				 * Deserializes received arcs to the local distributed graph.
				 *
				 * GhostArcs and GhostNodes are built as necessary to reproduce
				 * the graph structure, even if some imported arcs are linked
				 * to distant nodes.
				 *
				 * However, the data contained in those ghost nodes is not
				 * received yet : it will eventually be with the next ghost
				 * node synchronization operation, performed as a new
				 * Zoltan_Migrate cycle.
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

					std::set<unsigned long> ghostNodesIds;
					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					
					unsigned long localNode = graph->getNodes().begin()->first;

					for (int i = 0; i < num_ids; ++i) {
						read_zoltan_id(&global_ids[i * num_gid_entries]);
						json json_arc = json::parse(&buf[idx[i]]);

						// Json is unserialized in a temporary arc, with "fake"
						// nodes that just contains ID. We don't know yet which
						// nodes are on this local process or not.
						Arc<T> tempArc = json_arc.get<Arc<T>>();
						bool sourceNodeIsLocal = graph->getNodes().count(
									tempArc.getSourceNode()->getId()
									) > 0;
						bool targetNodeIsLocal = graph->getNodes().count(
									tempArc.getTargetNode()->getId()
									) > 0;

						if(!sourceNodeIsLocal || !targetNodeIsLocal) {
							// Source or target node is distant : we will
							// temporarily create a corresponding fake node
							if(sourceNodeIsLocal) {
								// The source node of the received arc is
								// contained in the local graph, so the target
								// node is distant, and has not been temporarily
								// created yet.

								// So adds it TEMPORARILY to the graph
								graph->buildNode(tempArc.getTargetNode()->getId());
								ghostNodesIds.insert(tempArc.getTargetNode()->getId());
							}
							else {
								// The target node of the received arc is
								// contained in the local graph, so the source
								// node is distant, and has not been
								// temporarily created yet.
								// So adds it TEMPORARILY to the graph
								graph->buildNode(tempArc.getSourceNode()->getId());
								ghostNodesIds.insert(tempArc.getSourceNode()->getId());
							}
						}
						// From there, potentially required temporary fake
						// nodes (corresponding to distant nodes) have been
						// created, so now source and target nodes of tempArc
						// arc local, so we can build it locally.
						// Source and target nodes of the tempArc are deleted
						// properly by the graph::link(Arc<T> tempArc).
						((DistributedGraph<T>*) data)->link(tempArc);
					} // for(i = 0; i < num_ids; i++)

					for(auto ghostNodeId : ghostNodesIds) {
						// Builds the ghosts from the temporary nodes added
						// earlier to the graph. This will also build the
						// corresponding ghost arc.
						// See the graph::buildGhostNode doc for more info.
						// Notice that the created ghost node does not contains
						// any data, but just represents the associated graph
						// structure. The corresponding data is eventually
						// retrieved later when the ghost nodes are
						// synchronized using a new Zoltan_Migrate cycle.

						//TODO: real proc number handling
						graph->buildGhostNode(*graph->getNode(ghostNodeId), 0);

						// We can now remove the temporary node and its
						// associated temporary links
						graph->removeNode(ghostNodeId);

					}

				}

			}
		}
	}
}

#endif
