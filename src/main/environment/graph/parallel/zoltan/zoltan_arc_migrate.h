#ifndef ZOLTAN_ARC_MIGRATE_H
#define ZOLTAN_ARC_MIGRATE_H

#include "zoltan_cpp.h"

#include "../distributed_graph.h"
#include "../olz.h"

using FPMAS::graph::Arc;
using FPMAS::graph::DistributedGraph;

namespace FPMAS {
	namespace graph {

		template<class T> class GhostNode;

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

							// One of the two node might be exported while the
							// other stay local, the two nodes might be
							// exported to the same proc, or the two nodes
							// might be exported to different proc.
							// TODO : origin and location transmission might be
							// optimized depending on those cases, because this
							// information might be deduced locally on the
							// destination proc in some cases. For now, we
							// always send the information.

							// It is required to send node origin
							// information to the destination proc, when a
							// ghost node will be created for target or origin,
							// to allow the destination proc to fetch those
							// nodes data
							unsigned long targetId = arc->getTargetNode()->getId();
							json_node["target"] = {
								graph->getProxy()->getOrigin(
										targetId
										),
								graph->getProxy()->getCurrentLocation(
										targetId
										)
							};

							unsigned long sourceId = arc->getSourceNode()->getId();
							json_node["source"] = {
								graph->getProxy()->getOrigin(
										sourceId
										),
								graph->getProxy()->getCurrentLocation(
										sourceId
										)
							};

							
							// Finally, serialize the node with the eventual
							// aditionnal fields
							std::string serial_node = json_node.dump();

							sizes[i] = serial_node.size() + 1;

							// Updates the cache
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
					std::unordered_map<unsigned long, std::string>* serial_cache
						= &graph->arc_serialization_cache;
					for (int i = 0; i < num_ids; ++i) {
						// Rebuilt node id
						unsigned long id = read_zoltan_id(&global_ids[i * num_gid_entries]);

						// Retrieves the serialized node
						std::string arc_str = serial_cache->at(id);
						for(int j = 0; j < arc_str.size(); j++) {
							buf[idx[i] + j] = arc_str[j];
						}
						buf[idx[i] + arc_str.size()] = 0; // str final char
					}
					// Clears the cache : all objects have been packed
					serial_cache->clear();
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

					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;
					
					for (int i = 0; i < num_ids; ++i) {
						read_zoltan_id(&global_ids[i * num_gid_entries]);
						json json_arc = json::parse(&buf[idx[i]]);

						// Json is unserialized in a temporary arc, with "fake"
						// nodes that just contains ID. We don't know yet which
						// nodes are on this local process or not.
						Arc<T> tempArc = json_arc.get<Arc<T>>();

						unsigned long sourceId = tempArc.getSourceNode()->getId();
						bool sourceNodeIsLocal = graph->getNodes().count(
								sourceId
									) > 0;

						unsigned long targetId = tempArc.getTargetNode()->getId();
						bool targetNodeIsLocal = graph->getNodes().count(
									targetId
									) > 0;

						if(!sourceNodeIsLocal || !targetNodeIsLocal) {
							// Source or target node is distant : a GhostNode must be used.
							if(sourceNodeIsLocal) {
								// The source node of the received arc is
								// contained in the local graph, so the target
								// node is distant
								GhostNode<T>* ghost;
								if(graph->getGhost()->getNodes().count(targetId) == 0) {
									// No ghost node as been created yet for
									// this node (from an other arc imported at
									// this period, or from an other period)
									ghost = graph->getGhost()->buildNode(targetId);

									// Updates the proxy from info received
									// with arc info, to know where the target
									// node can be found
									std::array<int, 2> targetLocation = json_arc.at("target").get<std::array<int, 2>>();
									graph->getProxy()->setOrigin(targetId, targetLocation[0]);
									graph->getProxy()->setCurrentLocation(targetId, targetLocation[1]);
								}
								else {
									// Use the existing GhostNode
									ghost = graph->getGhost()->getNodes().at(targetId);
								}
								// Links the ghost node with the local node
								// using a GhostArc
								graph->getGhost()->link(graph->getNode(sourceId), ghost, tempArc.getId());
							}
							else {
								// The target node of the received arc is
								// contained in the local graph, so the source
								// node is distant

								// Same process has above
								GhostNode<T>* ghost;
								if(graph->getGhost()->getNodes().count(sourceId) == 0) {
									ghost = graph->getGhost()->buildNode(sourceId);

									std::array<int, 2> sourceLocation = json_arc.at("source").get<std::array<int, 2>>();
									graph->getProxy()->setOrigin(sourceId, sourceLocation[0]);
									graph->getProxy()->setCurrentLocation(sourceId, sourceLocation[1]);
								}
								else {
									ghost = graph->getGhost()->getNodes().at(sourceId);
								}
								graph->getGhost()->link(ghost, graph->getNode(targetId), tempArc.getId());
							}
						}
						else {
							// Both nodes are local, no ghost needs to be used.
							// tempArc source and targetNodes are automatically
							// deleted by this function
							graph->link(
								sourceId,
								targetId,
								tempArc.getId()
								);
						}
						delete tempArc.getSourceNode();
						delete tempArc.getTargetNode();
					}
				}

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

					// The next steps will remove exported nodes from the local
					// graph, creating corresponding ghost nodes when necessary
					DistributedGraph<T>* graph = (DistributedGraph<T>*) data;

					// Computes the set of ids of exported nodes
					std::set<unsigned long> exportedNodeIds;
					for(int i = 0; i < graph->export_node_num; i++) {
						exportedNodeIds.insert(zoltan::utils::read_zoltan_id(
									&graph->export_node_global_ids[i * num_gid_entries])
								);
					}

					// Computes which ghost nodes must be created.
					// For each exported node, a ghost node is created if and only
					// if at least one local node is still connected to the
					// exported node.
					std::vector<Node<T>*> ghostNodesToBuild;
					for(auto id : exportedNodeIds) {
						Node<T>* node = graph->getNode(id);
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
							ghostNodesToBuild.push_back(graph->getNode(id));
						}
					}
					// Builds the ghost nodes
					for(auto node : ghostNodesToBuild) {
						// exportedNodeIds are ignored when creating links, so
						// that no link between ghost nodes will be created
						graph->getGhost()->buildNode(*node, exportedNodeIds);
					}

					// Remove nodes and collect fossils
					Fossil<T> ghostFossils;
					for(auto id : exportedNodeIds) {
						ghostFossils.merge(graph->removeNode(id));
					}

					// TODO : schema with fossils example
					// For info, see the Fossil and removeNode docs
					for(auto arc : ghostFossils.arcs) {
						graph->getGhost()->clearArc(arc);
					}
				}

			}
		}
	}
}

#endif
