#ifndef FPMAS_ZOLTAN_LOAD_BALANCING_H
#define FPMAS_ZOLTAN_LOAD_BALANCING_H

/** \file src/fpmas/graph/zoltan_load_balancing.h
 * ZoltanLoadBalancing implementation.
 */

#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/communication/communication.h"
#include "zoltan_cpp.h"

#include <set>

namespace fpmas { namespace graph {
	using api::graph::PartitionMap;
	using api::graph::NodeMap;

	/**
	 * Namespace containing Zoltan query functions definitions and utilities.
	 */
	namespace zoltan {
		/**
		 * Utility type that is used to store data and temporary buffers passed
		 * to Zoltan query functions as the `void* data` parameter.
		 */
		template<typename T>
		struct ZoltanData {
			/**
			 * Maps of nodes currently partitionned by Zoltan.
			 */
			NodeMap<T> node_map;

			/**
			 * Contains ids of **all** the nodes currently partitionned by
			 * Zoltan, including ones owned by other processes.
			 */
			std::set<fpmas::api::graph::DistributedId> distributed_node_ids;

			/**
			 * Nodes buffer. (built by Zoltan query functions)
			 *
			 * @see obj_list()
			 */
			std::vector<fpmas::api::graph::DistributedNode<T>*> nodes;

			/**
			 * Edges buffer. (built by Zoltan query functions)
			 *
			 * @see edge_list_multi_fn()
			 */
			std::vector<std::vector<fpmas::api::graph::DistributedNode<T>*>> target_nodes;
			/**
			 * Edge weights buffer. (built by Zoltan query functions)
			 *
			 * This buffer as exactly the same shape as edges.
			 *
			 * @see edge_list_multi_fn()
			 */
			std::vector<std::vector<float>> edge_weights;
		};

		/**
		 * In this case, a single ZOLTAN_ID_TYPE instance is enough to store an
		 * FPMAS_ID_TYPE.
		 *
		 * So num_gid_entries() returns `2`, to store an
		 * fpmas::api::graph::DistributedId instance (including the "rank"
		 * part).
		 */
		template<
			typename IdType = FPMAS_ID_TYPE,
					 typename std::enable_if<(sizeof(IdType) <= sizeof(ZOLTAN_ID_TYPE)), bool>::type = true>
						 static constexpr int num_gid_entries() {
							 return 2;
						 }
		/**
		 * In this case, a single ZOLTAN_ID_TYPE instance is **not** enough to store an
		 * FPMAS_ID_TYPE.
		 *
		 * Since ZOLTAN_ID_TYPE and FPMAS_ID_TYPE sizes are necessarily 16, 32
		 * or 64, `sizeof(FPMAS_ID_TYPE) / sizeof(ZOLTAN_ID_TYPE)` is
		 * necessarily an integer, and represents the number of
		 * ZOLTAN_ID_TYPE instances that are required to store a single
		 * FPMAS_ID_TYPE. For example, if `sizeof(ZOLTAN_ID_TYPE)` is 32
		 * and `sizeof(FPMAS_ID_TYPE)` is 64, we need 2 ZOLTAN_ID_TYPE
		 * instances to store a single FPMAS_ID_TYPE.
		 *
		 * So num_gid_entries() returns `sizeof(FPMAS_ID_TYPE) /
		 * sizeof(ZOLTAN_ID_TYPE) + 1` to store an
		 * fpmas::api::graph::DistributedId instance (including the "rank"
		 * part).
		 */
		template<
			typename IdType = FPMAS_ID_TYPE,
					 typename std::enable_if<(sizeof(IdType) > sizeof(ZOLTAN_ID_TYPE)), bool>::type = true>
						 static constexpr int num_gid_entries() {
							 return sizeof(FPMAS_ID_TYPE) / sizeof(ZOLTAN_ID_TYPE) + 1;
						 }

		/**
		 * NUM_GID_ENTRIES Zoltan parameter.
		 *
		 * The value is automatically computed to be compatible with the
		 * FPMAS_ID_TYPE.
		 */
		constexpr int NUM_GID_ENTRIES = num_gid_entries<>();

		/**
		 * Applies the FPMAS pre-defined Zoltan configuration to the provided
		 * Zoltan instance.
		 *
		 * @param zoltan pointer to zoltan instance to configure
		 */
		void zoltan_config(Zoltan* zoltan);

		/**
		 * Helper function to rebuild a regular node or edge id, as an
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
		DistributedId read_zoltan_id(const ZOLTAN_ID_PTR global_ids);

		/**
		 * Writes zoltan global ids to the specified `global_ids` adress.
		 *
		 * The original `unsigned long` is decomposed into 2 `unsigned int` to
		 * fit the default Zoltan data structure. The written id can then be
		 * rebuilt using the node_id(const ZOLTAN_ID_PTR) function.
		 *
		 * @param id id to write
		 * @param global_ids adress to a Zoltan global_ids array
		 */
		void write_zoltan_id(DistributedId id, ZOLTAN_ID_PTR global_ids);

		/**
		 * Returns the number of nodes currently managed by the current processor
		 * (i.e. the number of nodes contained in the local DistributedGraphBase
		 * instance).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_OBJ_FN).
		 *
		 * @param data user data (current NodeMap)
		 * @return number of nodes managed by the current process
		 */
		template<typename T> int num_obj(void *data, int*) {
			return ((ZoltanData<T>*) data)->node_map.size();
		}

		/**
		 * Lists node ids contained on the current process (i.e. in the
		 * DistributedGraphBase instance associated to the process).
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_OBJ_LIST_FN).
		 *
		 * @param data user data (current NodeMap)
		 * @param num_gid_entries number of entries used to describe global ids
		 * @param global_ids output : global ids assigned to processor
		 * @param obj_wgts output : weights list
		 */
		template<typename T> void obj_list(
				void *data,
				int num_gid_entries, 
				int, // num_lid_entries (unused)
				ZOLTAN_ID_PTR global_ids,
				ZOLTAN_ID_PTR, // local_ids (unused)
				int, // wgt_dim, 1 by default
				float *obj_wgts,
				int * // ierr (unused)
				) {
			ZoltanData<T>* z_data = (ZoltanData<T>*) data;
			int i = 0;
			for(auto n : z_data->node_map) {
				z_data->nodes.push_back(n.second);
				zoltan::write_zoltan_id(n.first, &global_ids[i * num_gid_entries]);
				obj_wgts[i++] = n.second->getWeight();
			}
		}

		/**
		 * Counts the number of outgoing edges of each node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (current NodeMap)
		 * @param num_obj number of objects IDs in global_ids
		 * @param num_edges output : number of outgoing edge for each node
		 */
		template<typename T> void num_edges_multi_fn(
				void *data,
				int, // num_gid_entries (unused)
				int, // num_lid_entries (unused)
				int num_obj,
				ZOLTAN_ID_PTR, // global_ids (unused)
				ZOLTAN_ID_PTR, // local_ids (unused)
				int *num_edges,
				int * // ierr (unused)
				) {
			struct NodeHandler {
				static void handle(
						ZoltanData<T>* z_data, int& count, int i, DistributedId node_id,
						api::graph::DistributedNode<T>* target_node, float edge_weight) {
					auto tgt_id = target_node->getId();
					// Incoming and outgoing edges are considered from all the
					// nodes, but should be treated once by all processes. (Notice
					// that the two nodes might be located on distinct processes)
					//
					// Considering the relation `rel` below, an edge is considered
					// only if `rel(node_id, tgt_id)` is true. Moreover, `rel` is
					// such as `rel(A, B) == true` XOR `rel(B, A) == true`: this
					// ensures that each edge is treated exactly once from A or B.
					// The first comparison on id() should ensure that no
					// preference is given on A or B for each edge. Moreover,
					// `rel(A, A) == false` so self edges are ignored.
					if(node_id.id() > tgt_id.id()
							|| (node_id.id() == tgt_id.id() && node_id.rank() > tgt_id.rank())
					  ) {
						// Considers the neighbor only if the load balancing
						// algorithm is also applied to it, potentially from an
						// other process.
						// The distributed_node_ids list contains the ids of ALL
						// the nodes that are currently balanced.
						// It is the responsability of
						// ZoltanLoadBalancing::balance() to perform communications
						// required to build this list, before the Zoltan algorithm
						// is effectively applied.
						if(z_data->distributed_node_ids.count(tgt_id) > 0) {
							auto& target_nodes = z_data->target_nodes[i];
							auto& edge_weights = z_data->edge_weights[i];
							auto it = std::find(
									target_nodes.begin(), target_nodes.end(),
									target_node
									);
							if(it == target_nodes.end()) {
								// No edges between node_id and tgt_id exists:
								// create a new one
								count++;
								target_nodes.push_back(target_node);
								edge_weights.push_back(edge_weight);
							} else {
								// An edge (outgoing or incoming) from node_id to
								// tgt_id already exists, so the weight of the
								// current edge is added to the existing one.
								edge_weights[std::distance(target_nodes.begin(), it)]
									+=edge_weight;
							}
						}
					}
				}
			};
			
			ZoltanData<T>* z_data = (ZoltanData<T>*) data;
			// Initializes `num_obj` empty vectors
			z_data->target_nodes.resize(num_obj);
			z_data->edge_weights.resize(num_obj);
			for(int i = 0; i < num_obj; i++) {
				auto node = z_data->nodes[i];
				auto node_id = node->getId();
				int count = 0;
				for(auto edge : node->getOutgoingEdges())
					NodeHandler::handle(
							z_data, count, i, node_id,
							edge->getTargetNode(), edge->getWeight()
							);
				for(auto edge : node->getIncomingEdges())
					NodeHandler::handle(
							z_data, count, i, node_id,
							edge->getSourceNode(), edge->getWeight()
							);

				num_edges[i] = count;
			}
		}

		/**
		 * Lists node IDs connected to each node through outgoing edges for each
		 * node.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_EDGES_MULTI_FN).
		 *
		 * @param data user data (current NodeMap)
		 * @param num_gid_entries number of entries used to describe global ids
		 * @param num_obj number of objects IDs in global_ids
		 * @param nbor_global_id output : neighbor ids for each node
		 * @param nbor_procs output : processor identifier of each neighbor in
		 * nbor_global_id
		 * @param ewgts output : edge weight for each neighbor
		 */
		template<typename T> void edge_list_multi_fn(
				void *data,
				int num_gid_entries,
				int, // num_lid_entries (unused)
				int num_obj,
				ZOLTAN_ID_PTR, // global_ids (unused)
				ZOLTAN_ID_PTR, // local_ids (unused)
				int *, // num_edges (unused)
				ZOLTAN_ID_PTR nbor_global_id,
				int *nbor_procs,
				int, // wgt_dim (unused, 1 by default)
				float *ewgts,
				int * // ierr (unused)
				) {

			ZoltanData<T>* z_data = (ZoltanData<T>*) data;

			int neighbor_index = 0;
			for (int i = 0; i < num_obj; ++i) {
				for(std::size_t j = 0; j < z_data->target_nodes[i].size(); j++) {
					auto target = z_data->target_nodes[i][j];
					zoltan::write_zoltan_id(
							target->getId(),
							&nbor_global_id[neighbor_index * num_gid_entries]
							);

					nbor_procs[neighbor_index]
						= target->location();

					ewgts[neighbor_index] = z_data->edge_weights[i][j];
					neighbor_index++;
				}
			}
		}

		/**
		 * Counts the number of fixed vertices.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_NUM_FIXED_OBJ_FN).
		 *
		 * @param data user data (current fixed NodeMap)
		 */
		template<typename T> int num_fixed_obj_fn(void* data, int*) {
			return ((NodeMap<T>*) data)->size();
		}

		/**
		 * Lists fixed vertices IDs.
		 *
		 * For more information about this function, see the [Zoltan
		 * documentation](https://cs.sandia.gov/Zoltan/ug_html/ug_query_lb.html#ZOLTAN_FIXED_OBJ_LIST_FN).
		 *
		 * @param data user data (current fixed NodeMap)
		 * @param num_gid_entries number of entries used to describe global ids
		 * @param fixed_gids output : fixed vertices ids list
		 * @param fixed_parts output : parts to which each fixed vertices is associated.
		 * In out context, corresponds to the process rank to which each
		 * vertice is attached.
		 */
		template<typename T> void fixed_obj_list_fn(
				void *data,
				int, // num_fixed_obj (unused)
				int num_gid_entries,
				ZOLTAN_ID_PTR fixed_gids,
				int *fixed_parts,
				int * // ierr (unused)
				) {
			PartitionMap* fixed_nodes = (PartitionMap*) data;
			int i = 0;
			for(auto fixed_node : *fixed_nodes) {
				zoltan::write_zoltan_id(fixed_node.first, &fixed_gids[i * num_gid_entries]);
				fixed_parts[i] = fixed_node.second;
				i++;
			}
		}
	}

	/**
	 * api::load_balancing::FixedVerticesLoadBalancing implementation based on
	 * the [Zoltan library](https://cs.sandia.gov/Zoltan/Zoltan.html).
	 */
	template<typename T>
		class ZoltanLoadBalancing : public api::graph::LoadBalancing<T>, public api::graph::FixedVerticesLoadBalancing<T> {
			private:
				struct ConcatSet {
					std::set<DistributedId> operator()(
							const std::set<DistributedId> s1,
							const std::set<DistributedId> s2
							) {
						std::set<DistributedId> s;
						s.insert(s1.begin(), s1.end());
						s.insert(s2.begin(), s2.end());
						return s;
					}
				};

				//Zoltan instance
				Zoltan zoltan;

				// Number of edges to export.
				int export_edges_num;

				// Edge ids to export buffer.
				ZOLTAN_ID_PTR export_edges_global_ids;
				// Edge export procs buffer.
				int* export_edges_procs;

				void setUpZoltan();

				zoltan::ZoltanData<T> zoltan_data;
				PartitionMap fixed_vertices;
				api::communication::MpiCommunicator& comm;
				communication::TypedMpi<std::set<DistributedId>> id_mpi;

			public:

				/**
				 * ZoltanLoadBalancing constructor.
				 *
				 * @param comm MpiCommunicator implementation
				 */
				ZoltanLoadBalancing(communication::MpiCommunicatorBase& comm)
					: zoltan(comm.getMpiComm()), comm(comm), id_mpi(comm) {
						setUpZoltan();
					}

				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMap fixed_vertices
						) override;

				/**
				 * \copydoc fpmas::api::graph::FixedVerticesLoadBalancing::balance(NodeMap<T>, api::graph::PartitionMap, api::graph::PartitionMode)
				 *
				 * \implem
				 * Computes a balanced partition from the default
				 * [Zoltan PHG Hypergraph partitioning method](https://cs.sandia.gov/Zoltan/ug_html/ug_alg_phg.html)
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMap fixed_vertices,
						api::graph::PartitionMode partition_mode
						) override;
				
				PartitionMap balance(api::graph::NodeMap<T> nodes) override;

				/**
				 * \copydoc fpmas::api::graph::LoadBalancing::balance(NodeMap<T>, api::graph::PartitionMode)
				 *
				 * \implem
				 * Equivalent to balance(NodeMap<T>, PartitionMap, PartitionMode),
				 * with an empty set of fixed vertices.
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMode partition_mode) override;
		};

	template<typename T> PartitionMap
		ZoltanLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes,
				api::graph::PartitionMap fixed_vertices
				) {
			return balance(nodes, fixed_vertices, api::graph::PARTITION);
		}

	template<typename T> PartitionMap
		ZoltanLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes,
				api::graph::PartitionMap fixed_vertices,
				api::graph::PartitionMode partition_mode
				) {
			switch(partition_mode) {
				case api::graph::PARTITION:
					this->zoltan.Set_Param("LB_APPROACH", "PARTITION");
					break;
				case api::graph::REPARTITION:
					this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
					break;
			}

			for(auto local_node : nodes)
				zoltan_data.distributed_node_ids.insert(local_node.first);

			// Fetches ids of **all** the nodes that are currently partitionned
			zoltan_data.distributed_node_ids = communication::all_reduce(
					id_mpi, zoltan_data.distributed_node_ids, ConcatSet()
					);


			// Moves the temporary node map into `zoltan_data`. This is safe,
			// since `nodes` is not reused in this scope.
			zoltan_data.node_map = std::move(nodes);

			this->fixed_vertices = fixed_vertices;
			int changes;
			int num_lid_entries;
			int num_gid_entries;


			int num_import; 
			ZOLTAN_ID_PTR import_global_ids;
			ZOLTAN_ID_PTR import_local_ids;
			int * import_procs;
			int * import_to_part;

			int export_node_num;
			ZOLTAN_ID_PTR export_node_global_ids;
			ZOLTAN_ID_PTR export_local_ids;
			int* export_node_procs;
			int* export_to_part;

			// Computes Zoltan partitioning
			this->zoltan.LB_Partition(
					changes,
					num_gid_entries,
					num_lid_entries,
					num_import,
					import_global_ids,
					import_local_ids,
					import_procs,
					import_to_part,
					export_node_num,
					export_node_global_ids,
					export_local_ids,
					export_node_procs,
					export_to_part
					);

			PartitionMap partition;
			for(int i = 0; i < export_node_num; i++) {
				partition[zoltan::read_zoltan_id(&export_node_global_ids[i * num_gid_entries])]
					= export_node_procs[i];
			}
			this->zoltan.LB_Free_Part(
					&import_global_ids,
					&import_local_ids,
					&import_procs,
					&import_to_part
					);

			this->zoltan.LB_Free_Part(
					&export_node_global_ids,
					&export_local_ids,
					&export_node_procs,
					&export_to_part
					);

			// Clears `zoltan_data`
			zoltan_data.node_map.clear();
			zoltan_data.distributed_node_ids.clear();
			zoltan_data.nodes.clear();
			zoltan_data.target_nodes.clear();
			zoltan_data.edge_weights.clear();

			return partition;
		}

	template<typename T> PartitionMap
		ZoltanLoadBalancing<T>::balance(api::graph::NodeMap<T> nodes) {
			return this->balance(nodes, api::graph::PARTITION);
		}

	template<typename T> PartitionMap
		ZoltanLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes, api::graph::PartitionMode partition_mode) {
			return this->balance(nodes, {}, partition_mode);
		}

	/*
	 * Initializes zoltan parameters and zoltan lb query functions.
	 */
	template<typename T> void ZoltanLoadBalancing<T>::setUpZoltan() {
		zoltan::zoltan_config(&this->zoltan);

		// Initializes Zoltan Node Load Balancing functions
		this->zoltan.Set_Num_Fixed_Obj_Fn(zoltan::num_fixed_obj_fn<T>, &this->fixed_vertices);
		this->zoltan.Set_Fixed_Obj_List_Fn(zoltan::fixed_obj_list_fn<T>, &this->fixed_vertices);
		this->zoltan.Set_Num_Obj_Fn(zoltan::num_obj<T>, &this->zoltan_data);
		this->zoltan.Set_Obj_List_Fn(zoltan::obj_list<T>, &this->zoltan_data);
		this->zoltan.Set_Num_Edges_Multi_Fn(zoltan::num_edges_multi_fn<T>, &this->zoltan_data);
		this->zoltan.Set_Edge_List_Multi_Fn(zoltan::edge_list_multi_fn<T>, &this->zoltan_data);
	}

}}
#endif
