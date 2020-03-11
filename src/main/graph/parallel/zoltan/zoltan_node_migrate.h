#ifndef ZOLTAN_NODE_MIGRATE_H
#define ZOLTAN_NODE_MIGRATE_H

#include <set>

#include "zoltan_cpp.h"

#include "utils/macros.h"
#include "zoltan_utils.h"
#include "../synchro/sync_mode.h"
#include "../synchro/no_sync_mode.h"
#include "../../base/node.h"


using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

namespace FPMAS::graph::parallel {

	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	using base::NodeId;
	using base::ArcId;

	using synchro::wrappers::SyncData;
	using synchro::modes::NoSyncMode;

	using zoltan::utils::write_zoltan_id;
	using zoltan::utils::read_zoltan_id;

	/**
	 * The zoltan::node namespace defines functions used to migrate
	 * nodes using zoltan.
	 */
	namespace zoltan::node {
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
		template<typename T, int N, SYNC_MODE> void obj_size_multi_fn(
				void *data,
				int num_gid_entries,
				int num_lid_entries,
				int num_ids,
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR local_ids,
				int *sizes,
				int *ierr) {


			DistributedGraph<T, S, N>* graph = (DistributedGraph<T, S, N>*) data;
			std::unordered_map<NodeId, Node<std::unique_ptr<SyncData<T,N,S>>, N>*> nodes = graph->getNodes();
			for (int i = 0; i < num_ids; i++) {
				Node<std::unique_ptr<SyncData<T,N,S>>, N>* node = nodes.at(read_zoltan_id(&global_ids[i * num_gid_entries]));

				if(graph->node_serialization_cache.count(node->getId()) == 1) {
					sizes[i] = graph->node_serialization_cache.at(node->getId()).size()+1;
				}
				else {
					json json_node = *node;

					json_node["origin"] = graph->proxy.getOrigin(node->getId());
					json_node["from"] = graph->getMpiCommunicator().getRank();

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
		template<typename T, int N, SYNC_MODE> void pack_obj_multi_fn(
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

			DistributedGraph<T, S, N>* graph = (DistributedGraph<T, S, N>*) data;
			// The node should actually be serialized when computing
			// the required buffer size. For efficiency purpose, we temporarily
			// store the result and delete it when it is packed.
			std::unordered_map<NodeId, std::string>* serial_cache
				= &graph->node_serialization_cache;
			for (int i = 0; i < num_ids; ++i) {
				// Rebuilt node id
				NodeId id = read_zoltan_id(&global_ids[i * num_gid_entries]);
				// Retrieves the serialized node
				std::string node_str = serial_cache->at(id);

				// Copy str to zoltan buffer
				std::sprintf(&buf[idx[i]], "%s", node_str.c_str());
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
		 * @param num_ids number of nodes to unpack
		 * @param global_ids item global ids
		 * @param sizes buffer sizes for each object
		 * @param idx each object starting point in buf
		 * @param buf communication buffer
		 * @param ierr Result : error code
		 *
		 */
		template<typename T, int N, SYNC_MODE> void unpack_obj_multi_fn(
				void *data,
				int num_gid_entries,
				int num_ids,
				ZOLTAN_ID_PTR global_ids,
				int *sizes,
				int *idx,
				char *buf,
				int *ierr) {

			DistributedGraph<T, S, N>* graph = (DistributedGraph<T, S, N>*) data;
			for (int i = 0; i < num_ids; i++) {
				json json_node = json::parse(&buf[idx[i]]);

				// Node<LocalData<T>, N> node = json_node.get<Node<LocalData<T>, N>>();

				NodeId id = json_node.at("id").get<NodeId>();

				if(graph->getGhost().getNodes().count(id) > 0)
					graph->obsoleteGhosts.insert(id);

				graph->buildNode(
						id,
						json_node.at("weight").get<float>(),
						json_node.at("data").get<T>()
						);

				int origin = json_node.at("origin").get<int>();
				graph->proxy.setOrigin(id, origin);
				graph->proxy.setLocal(id);
			}

		}

		/**
		 * This function is called by Zoltan after node imports and
		 * exports have been performed.
		 *
		 * In our context, this functions computes arcs that need to be
		 * sent to each process according to exported nodes, and stores
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
		template<typename T, int N, SYNC_MODE> void post_migrate_pp_fn_olz(
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

			DistributedGraph<T, S, N>* graph = (DistributedGraph<T, S, N>*) data;

			std::unordered_map<NodeId, Node<std::unique_ptr<SyncData<T,N,S>>, N>*> nodes = graph->getNodes();
			// Set used to ensure that each arc is sent at most once to
			// each process.
			std::set<std::pair<ArcId, int>> exportedArcPairs;

			std::vector<Arc<std::unique_ptr<SyncData<T,N,S>>, N>*> arcsToExport;
			std::vector<int> procs; // Arcs destination procs

			for (int i =0; i < num_export; i++) {
				NodeId id = read_zoltan_id(&export_global_ids[i * num_gid_entries]);

				auto exported_node = nodes.at(id);
				int dest_proc = export_procs[i];

				// Updates Proxy
				graph->getProxy().setCurrentLocation(id, dest_proc);

				// Computes arc exports
				for(auto layer : exported_node->getLayers()) {
					for(auto arc : layer.getIncomingArcs()) {
						std::pair<ArcId, int> arc_proc_pair =
							std::pair<ArcId, int>(
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
				for(auto layer : exported_node->getLayers()) {
					for(auto arc : layer.getOutgoingArcs()) {
						std::pair<ArcId, int> arc_proc_pair =
							std::pair<ArcId, int>(
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
			}
			graph->export_arcs_num = arcsToExport.size();

			graph->export_arcs_global_ids = (ZOLTAN_ID_PTR) std::malloc(sizeof(ZOLTAN_ID_TYPE) * graph->export_arcs_num * num_gid_entries);
			graph->export_arcs_procs = (int*) std::malloc(sizeof(int) * graph->export_arcs_num);

			for(int i = 0; i < graph->export_arcs_num; i++) {
				write_zoltan_id(arcsToExport.at(i)->getId(), &graph->export_arcs_global_ids[i * num_gid_entries]);
				graph->export_arcs_procs[i] = procs[i];
			}
		}

		/**
		 * This function is called by Zoltan after node imports and
		 * exports have been performed.
		 *
		 * In our context, this functions computes arcs that need to be
		 * sent to each process according to exported nodes, and stores
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
		template<typename T, int N> void post_migrate_pp_fn_no_sync(
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

			DistributedGraph<T, NoSyncMode, N>* graph =(DistributedGraph<T, NoSyncMode, N>*) data;

			std::vector<Arc<std::unique_ptr<SyncData<T,N,NoSyncMode>>, N>*> arcsToExport;
			std::vector<int> procs; // Arcs destination procs

			std::unordered_map<NodeId, int> nodeDestinations;

			for (int i = 0; i < num_export; i++) {
				nodeDestinations[read_zoltan_id(&export_global_ids[i * num_gid_entries])] = export_procs[i];
			}

			for (auto nodeDest : nodeDestinations) {
				auto exported_node = graph->getNodes().at(nodeDest.first);

				// Updates Proxy for consistency, even if should no ghost are used
				graph->getProxy().setCurrentLocation(nodeDest.first, nodeDest.second);

				// Computes arc exports
				// We only need to check incoming arcs for each node :
				// this ensures that each arc is exported exactly once
				// if necessary. Indeed, the required outgoing arcs for
				// each node will be exported as incoming arcs of
				// corresponding target nodes, or ignored otherwise,
				// what is the required behavior.
				for(auto layer : exported_node->getLayers()) {
					for(auto arc : layer.getIncomingArcs()) {
						NodeId sourceId = arc->getSourceNode()->getId();
						if(nodeDestinations.count(sourceId) > 0 // The source is also exported
								&& nodeDestinations.at(sourceId) == nodeDest.second// The source is exported to the same proc as the target node
						) {
							arcsToExport.push_back(arc);
							// Arcs will be sent to the proc and part associated to
							// the current node
							procs.push_back(nodeDest.second);
						}
					}
				}
			}
			graph->export_arcs_num = arcsToExport.size();

			graph->export_arcs_global_ids = (ZOLTAN_ID_PTR) std::malloc(sizeof(ZOLTAN_ID_TYPE) * graph->export_arcs_num * num_gid_entries);
			graph->export_arcs_procs = (int*) std::malloc(sizeof(int) * graph->export_arcs_num);

			for(int i = 0; i < graph->export_arcs_num; i++) {
				write_zoltan_id(arcsToExport.at(i)->getId(), &graph->export_arcs_global_ids[i * num_gid_entries]);
				graph->export_arcs_procs[i] = procs[i];
			}
		}
	}
}
#endif
