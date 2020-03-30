#ifndef ZOLTAN_ARC_MIGRATE_H
#define ZOLTAN_ARC_MIGRATE_H

#include "zoltan_cpp.h"

#include "../olz.h"
#include "../synchro/no_sync_mode.h"

using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	template<typename T, SYNC_MODE> class DistributedGraphBase;
	template<typename T, SYNC_MODE> class GhostNode;

	using synchro::modes::NoSyncMode;
	using synchro::wrappers::SyncData;

	namespace zoltan {
		/**
		 * The zoltan::arc namespace defines functions used to migrate
		 * arcs using zoltan.
		 * The arc migration process is only performed right after nodes have
		 * migrated.
		 */
		namespace arc {
			/**
			 * Computes the buffer sizes required to serialize nodes corresponding
			 * to global_ids.
			 *
			 * For more information about this function, see the [Zoltan
			 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_OBJ_SIZE_MULTI_FN).
			 *
			 * @param data user data (local DistributedGraphBase instance)
			 * @param num_gid_entries number of entries used to describe global ids (should be 2)
			 * @param num_lid_entries number of entries used to describe local ids (should be 0)
			 * @param num_ids number of nodes to serialize
			 * @param global_ids global ids of nodes to serialize
			 * @param local_ids unused
			 * @param sizes Result : buffer sizes for each node
			 * @param ierr Result : error code
			 */
			template<typename T, SYNC_MODE> void obj_size_multi_fn(
					void *data,
					int num_gid_entries,
					int num_lid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					ZOLTAN_ID_PTR local_ids,
					int *sizes,
					int *ierr) {


				DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "obj_size_multi_fn");
				std::unordered_map<DistributedId, Arc<std::unique_ptr<SyncData<T,S>>, DistributedId>*> arcs = graph->getArcs();
				for (int i = 0; i < num_ids; i++) {
					DistributedId arcId = utils::read_zoltan_id(&global_ids[i * num_gid_entries]);
					Arc<std::unique_ptr<SyncData<T,S>>, DistributedId>* arc;
					try {
						arc = arcs.at(arcId);
					} catch (const std::exception& e) {
						arc = graph->getGhost().getArcs().at(arcId);
					}

					if(graph->arc_serialization_cache.count(arc->getId()) == 1) {
						sizes[i] = graph->arc_serialization_cache.at(arc->getId()).size() + 1;
					}
					else {
						json json_arc = *arc;

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
						DistributedId targetId = arc->getTargetNode()->getId();
						json_arc["target"] = {
							graph->getProxy().getOrigin(
									targetId
									),
							graph->getProxy().getCurrentLocation(
									targetId
									)
						};

						DistributedId sourceId = arc->getSourceNode()->getId();
						json_arc["source"] = {
							graph->getProxy().getOrigin(
									sourceId
									),
							graph->getProxy().getCurrentLocation(
									sourceId
									)
						};

						// Finally, serialize the node with the eventual
						// aditionnal fields
						std::string serial_arc = json_arc.dump();

						sizes[i] = serial_arc.size() + 1;

						// Updates the cache
						graph->arc_serialization_cache[arc->getId()] = serial_arc;
					}

				}

			}

			/**
			 * Serializes the input list of nodes as a json string.
			 *
			 * For more information about this function, see the [Zoltan
			 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_mig.html#ZOLTAN_PACK_OBJ_MULTI_FN).
			 *
			 * @param data user data (local DistributedGraphBase instance)
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
			template<typename T, SYNC_MODE> void pack_obj_multi_fn(
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

				DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "pack_obj_multi_fn");
				// The node should actually be serialized when computing
				// the required buffer size. For efficiency purpose, we temporarily
				// store the result and delete it when it is packed.
				std::unordered_map<DistributedId, std::string>* serial_cache
					= &graph->arc_serialization_cache;
				for (int i = 0; i < num_ids; ++i) {
					// Rebuilt arc id
					DistributedId id = utils::read_zoltan_id(&global_ids[i * num_gid_entries]);

					// Retrieves the serialized node
					std::string arc_str = serial_cache->at(id);
					Arc<std::unique_ptr<SyncData<T,S>>, DistributedId>* arc;
					try {
						arc = graph->getArcs().at(id);
					} catch (const std::exception& e) {
						arc = graph->getGhost().getArcs().at(id);
					}

					// Copy str to zoltan buffer
					std::sprintf(&buf[idx[i]], "%s", arc_str.c_str());
				}
				// Clears the cache : all objects have been packed
				serial_cache->clear();
			}

			/**
			 * Called once nodes migration is done, after arcs have been
			 * exported and before arcs are imported.
			 *
			 * If nodes have been imported and were already contained in
			 * the graph as ghost nodes, ghosts are replaced by real nodes
			 * in this Arc mid migrate process, once arcs linked to the
			 * ghost nodes have been exported if necessary.
			 *
			 * Actually, if the ghost node is removed directly when the
			 * real node is imported, while arcs have not migrated yet,
			 * arcs linked to the ghost node will be deleted even if they
			 * might have been exported in the arc migration process, what
			 * results in a significant loss of information.
			 *
			 * This process should be performed as a "mid" migrate (not
			 * post), so that imported arcs are built properly on the
			 * imported nodes, not on the obsolete ghosts.
			 */
			template <typename T, SYNC_MODE> void mid_migrate_pp_fn(
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
				DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "mid_migrate");
				for(auto nodeId : graph->obsoleteGhosts) {
					graph->getGhost().removeNode(nodeId);
				}
				graph->obsoleteGhosts.clear();
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
			 * @param data user data (local DistributedGraphBase instance)
			 * @param num_gid_entries number of entries used to describe global ids (should be 2)
			 * @param num_ids number of nodes to unpack
			 * @param global_ids item global ids
			 * @param sizes buffer sizes for each object
			 * @param idx each object starting point in buf
			 * @param buf communication buffer
			 * @param ierr Result : error code
			 *
			 */
			template<typename T, SYNC_MODE> void unpack_obj_multi_fn(
					void *data,
					int num_gid_entries,
					int num_ids,
					ZOLTAN_ID_PTR global_ids,
					int *sizes,
					int *idx,
					char *buf,
					int *ierr) {

				DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "unpack_obj_multi_fn");

				// The same arc can be imported multiple times from
				// different procs.
				// e.g : When nodes 1 and 0 are linked and live on different
				// procs, and they are both imported to an other proc.
				// This set records which arcs are imported and is used to
				// ignore already imported ones.
				// TODO: this might probably be optimized, removing
				// duplicates when import / export maps are computed
				std::set<DistributedId> receivedIds;

				for (int i = 0; i < num_ids; ++i) {
					json json_arc = json::parse(&buf[idx[i]]);

					// Unserialize and process the arc only if it has not
					// been imported already.
					if(receivedIds.count(json_arc.at("id").get<DistributedId>()) == 0) {

						// Json is unserialized in a temporary arc, with "fake"
						// nodes that just contains ID. We don't know yet which
						// nodes are on this local process or not.
						Arc<std::unique_ptr<SyncData<T,S>>, DistributedId> tempArc = json_arc.get<Arc<std::unique_ptr<SyncData<T,S>>, DistributedId>>();

						receivedIds.insert(tempArc.getId());

						DistributedId sourceId = tempArc.getSourceNode()->getId();
						bool sourceNodeIsLocal = graph->getNodes().count(
								sourceId
								) > 0;

						DistributedId targetId = tempArc.getTargetNode()->getId();
						bool targetNodeIsLocal = graph->getNodes().count(
								targetId
								) > 0;

						if(!sourceNodeIsLocal || !targetNodeIsLocal) {
							// Source or target node is distant : a GhostNode must be used.
							if(sourceNodeIsLocal) {
								// The source node of the received arc is
								// contained in the local graph, so the target
								// node is distant
								GhostNode<T, S>* ghost;
								if(graph->getGhost().getNodes().count(targetId) == 0) {
									// No ghost node as been created yet for
									// this node (from an other arc imported at
									// this period, or from an other period)
									ghost = graph->getGhost().buildNode(targetId);

									// Updates the proxy from info received
									// with arc info, to know where the target
									// node can be found
									std::array<int, 2> targetLocation = json_arc.at("target").get<std::array<int, 2>>();
									graph->getProxy().setOrigin(targetId, targetLocation[0]);
									graph->getProxy().setCurrentLocation(targetId, targetLocation[1]);
								}
								else {
									// Use the existing GhostNode
									ghost = graph->getGhost().getNodes().at(targetId);
								}
								// Links the ghost node with the local node
								// using a GhostArc
								graph->getGhost().link(graph->getNode(sourceId), ghost, tempArc.getId(), tempArc.layer);
							}
							else if (targetNodeIsLocal) {
								// The target node of the received arc is
								// contained in the local graph, so the source
								// node is distant

								// Same process has above
								GhostNode<T, S>* ghost;
								if(graph->getGhost().getNodes().count(sourceId) == 0) {
									ghost = graph->getGhost().buildNode(sourceId);

									std::array<int, 2> sourceLocation = json_arc.at("source").get<std::array<int, 2>>();
									graph->getProxy().setOrigin(sourceId, sourceLocation[0]);
									graph->getProxy().setCurrentLocation(sourceId, sourceLocation[1]);
								}
								else {
									ghost = graph->getGhost().getNodes().at(sourceId);
								}
								graph->getGhost().link(ghost, graph->getNode(targetId), tempArc.getId(), tempArc.layer);
							}
						}
						else {
							// Both nodes are local, no ghost needs to be used.
							graph->_link(
									tempArc.getId(),
									sourceId,
									targetId,
									tempArc.layer
									);
						}
						delete tempArc.getSourceNode();
						delete tempArc.getTargetNode();
					}
				}
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "unpack_obj_multi_fn : done");
			}

			/**
			 * Called once nodes and arcs migration processes are done when
			 * using the SyncMode::OLZ synchronization mode.
			 *
			 * This process builds required ghost nodes and deletes useless
			 * ones according to nodes that were just exported.
			 */
			template<typename T, SYNC_MODE> void post_migrate_pp_fn_olz(
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
				DistributedGraphBase<T, S>* graph = (DistributedGraphBase<T, S>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "post_migrate_pp_fn_olz");

				// Computes the set of ids of exported nodes
				std::set<DistributedId> exportedIds;
				for(int i = 0; i < graph->export_node_num; i++) {
					exportedIds.insert(zoltan::utils::read_zoltan_id(
								&graph->export_node_global_ids[i * num_gid_entries])
							);
				}

				// Builds ghost nodes when necessary.
				// For each exported node, a ghost node is created if and only
				// if at least one local node is still connected to the
				// exported node.
				for(auto id : exportedIds) {
					Node<std::unique_ptr<SyncData<T,S>>, DistributedId>* node = graph->getNode(id);
					bool buildGhost = false;
					for(auto& layer : node->getLayers()) {
						for(auto arc : layer.second.getOutgoingArcs()) {
							if(exportedIds.count(arc->getTargetNode()->getId()) == 0) {
								buildGhost = true;
								break;
							}
						}
					}
					if(!buildGhost) {
						for(auto& layer : node->getLayers()) {
							for(auto arc : layer.second.getIncomingArcs()) {
								if(exportedIds.count(arc->getSourceNode()->getId()) == 0) {
									buildGhost = true;
									break;
								}
							}
						}
					}
					if(buildGhost) {
						FPMAS_LOGD(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "Building ghost node %lu", node->getId());
						graph->getGhost().buildNode(*node, exportedIds);
					}
				}

				// Remove nodes
				for(auto id : exportedIds) {
					FPMAS_LOGD(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "Removing exported node %lu", id);
					graph->removeExportedNode(id);
				}

				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "post_migrate_pp_fn_olz : done");
			}

			/**
			 * Called once nodes and arcs migration processes are done when
			 * using the synchro::None synchronization mode.
			 *
			 * In this mode, the only thing to do is deleting the exported
			 * nodes from the local graph.
			 */
			template<typename T> void post_migrate_pp_fn_no_sync(
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
				DistributedGraphBase<T, NoSyncMode>* graph = (DistributedGraphBase<T, NoSyncMode>*) data;
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "post_migrate_pp_fn_no_sync");

				// Removes exported nodes from the local graph
				for(int i = 0; i < graph->export_node_num; i++) {
					graph->removeNode(zoltan::utils::read_zoltan_id(
								&graph->export_node_global_ids[i * num_gid_entries])
							);
				}
				FPMAS_LOGV(graph->getMpiCommunicator().getRank(), "ZOLTAN_ARC", "post_migrate_pp_fn_no_sync : done");
			}
		}
	}
}

#endif
