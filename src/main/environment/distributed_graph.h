#ifndef DISTRIBUTED_GRAPH_H
#define DISTRIBUTED_GRAPH_H

#include "graph.h"
#include "zoltan_cpp.h"

namespace FPMAS {
	namespace graph {
		/**
		 * The FPMAS::graph::zoltan namespace contains definitions of all the
		 * required Zoltan query functions to compute and distribute graph
		 * partitions.
		 */
		namespace zoltan {
			template<class T> int num_obj(void *data, int* ierr);
			template<class T> void obj_list(
					void *, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int, float *, int *
					);
			template<class T> void num_edges_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
					);
			template<class T> void edge_list_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, ZOLTAN_ID_PTR, int *, int, float *, int *
					);
			template<class T> void obj_size_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *
					); 

			template<class T> void pack_obj_multi_fn(
					void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
					);

			template<class T> void unpack_obj_multi_fn(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);
		}

		template<class T> class DistributedGraph : public Graph<T> {
			friend void zoltan::obj_size_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *); 
			friend void zoltan::pack_obj_multi_fn<T>(
				void *, int, int, int, ZOLTAN_ID_PTR, ZOLTAN_ID_PTR, int *, int *, int *, char *, int *
				);
			friend void zoltan::unpack_obj_multi_fn<T>(
					void *, int, int, ZOLTAN_ID_PTR, int *, int *, char *, int *
					);

			private:
				Zoltan* zoltan;
				std::unordered_map<unsigned long, std::string> node_serialization_cache;

			public:
				DistributedGraph<T>(Zoltan*);

				void distribute();

		};

		/**
		 * DistributedGraph constructor.
		 *
		 * A DistributedGraph is a special graph instance that can be
		 * distributed across available processors through the provided Zoltan
		 * instance.
		 *
		 * @param z pointer to an initiliazed zoltan instance
		 */
		template<class T> DistributedGraph<T>::DistributedGraph(Zoltan* z)
			: Graph<T>(), zoltan(z) {
				zoltan->Set_Num_Obj_Fn(FPMAS::graph::zoltan::num_obj<T>, this);
				zoltan->Set_Obj_List_Fn(FPMAS::graph::zoltan::obj_list<T>, this);
				zoltan->Set_Num_Edges_Multi_Fn(FPMAS::graph::zoltan::num_edges_multi_fn<T>, this);
				zoltan->Set_Edge_List_Multi_Fn(FPMAS::graph::zoltan::edge_list_multi_fn<T>, this);

				zoltan->Set_Obj_Size_Multi_Fn(FPMAS::graph::zoltan::obj_size_multi_fn<T>, this);
				zoltan->Set_Pack_Obj_Multi_Fn(FPMAS::graph::zoltan::pack_obj_multi_fn<T>, this);
				zoltan->Set_Unpack_Obj_Multi_Fn(FPMAS::graph::zoltan::unpack_obj_multi_fn<T>, this);

			}

		namespace zoltan {

			/**
			 * Convenient function to rebuild a regular node id, as an
			 * unsigned long, from a ZOLTAN_ID_PTR global id array, that actually
			 * stores 2 unsigned int for each node unsigned long id.
			 * So, with our configuration, we use 2 unsigned int in Zoltan to
			 * represent each id. The `i` input parameter should correspond to the
			 * first part index of the node to query in the `global_ids` array. In
			 * consequence, `i` should actually always be even.
			 *
			 * @param global_ids an adress to a pair of Zoltan global ids
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

			/**
			 * Writes zoltan global ids to the specified `global_ids` adress.
			 *
			 * The original `unsigned long` is decomposed into 2 `unsigned int` to
			 * fit the default Zoltan data structure. The written id can then be
			 * rebuilt using the node_id(const ZOLTAN_ID_PTR) function.
			 *
			 * @param node_id node id to write
			 * @param global_ids adress to a Zoltan global_ids array
			 */
			void write_zoltan_node_id(unsigned long node_id, ZOLTAN_ID_PTR global_ids) {
				global_ids[0] = (node_id & 0xFFFF0000) >> 16 ;
				global_ids[1] = (node_id & 0xFFFF);
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
			template<class T> int num_obj(void *data, int* ierr) {
				return ((DistributedGraph<T>*) data)->getNodes().size();
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
			template<class T> void obj_list(
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
			template<class T> void num_edges_multi_fn(
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
			template<class T> void edge_list_multi_fn(
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


				std::unordered_map<unsigned long, Node<T>*> nodes = ((DistributedGraph<T>*) data)->getNodes();
				for (int i = 0; i < num_ids; i++) {
					Node<T>* node = nodes.at(node_id(&global_ids[i * num_gid_entries]));

					json json_node = *node;
					std::string serial_node = json_node.dump();

					sizes[i] = serial_node.size() + 1;

					((DistributedGraph<T>*) data)->node_serialization_cache[node->getId()] = serial_node;

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

				// The node should actually been already serialized when computing
				// the required buffer size. For efficiency purpose, we temporarily
				// store the result and delete it when it is packed.
				std::unordered_map<unsigned long, std::string> serial_cache
					= ((DistributedGraph<T>*) data)->node_serialization_cache;
				for (int i = 0; i < num_ids; ++i) {
					// Rebuilt node id
					unsigned long id = node_id(&global_ids[i * num_gid_entries]);

					// Retrieves the serialized node
					std::string node_str = serial_cache.at(id);
					for(int j = 0; j < sizes[i] - 1; j++) {
						buf[idx[i] + j] = node_str[j];
					}
					buf[idx[i] + sizes[i] - 1] = 0; // str final char

					// Removes entry from the serialization buffer
					serial_cache.erase(id);
				}

			}

			/**
			 *
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

				for (int i = 0; i < num_ids; ++i) {
					node_id(&global_ids[i * num_gid_entries]);
					json json_node = json::parse(&buf[idx[i]]);

					((DistributedGraph<T>*) data)->buildNode(json_node.get<Node<T>>());
				}

			}
		}
	}
}
#endif
