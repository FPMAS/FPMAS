#ifndef ZOLTAN_LOAD_BALANCING_H
#define ZOLTAN_LOAD_BALANCING_H

#include "zoltan_cpp.h"
#include "api/graph/parallel/load_balancing.h"
#include "graph/parallel/proxy/proxy.h"
#include "utils/config.h"
#include "zoltan_lb.h"

namespace FPMAS::graph::parallel::zoltan {

	template<typename NodeType>
	struct NodesProxyPack {
		std::unordered_map<DistributedId, NodeType*>* const nodes;
		const proxy::Proxy* const proxy;
	};

	template<typename NodeType>
		class ZoltanLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<NodeType> {
			private:
				//Zoltan instance
				Zoltan zoltan;

				// Number of nodes to export.

		
				// Number of nodes to export.
				int export_arcs_num;

				// Arc ids to export buffer.
				ZOLTAN_ID_PTR export_arcs_global_ids;
				// Arc export procs buffer.
				int* export_arcs_procs;

				void setUpZoltan();

				std::unordered_map<DistributedId, NodeType*> nodes;
				std::unordered_map<DistributedId, int> fixedVertices;
				const proxy::Proxy& proxy;

				NodesProxyPack<NodeType> nodesProxyPack {&nodes, &proxy};


			public:
				ZoltanLoadBalancing(MPI_Comm comm, const proxy::Proxy& proxy)
					: zoltan(comm), proxy(proxy) {
					setUpZoltan();
				}

				const std::unordered_map<DistributedId, NodeType*> getNodes() const {
					return nodes;
				}
				/*
				 *const std::unordered_map<DistributedId, int> getFixedVertices() const {
				 *    return fixedVertices;
				 *}
				 */
				const proxy::Proxy& getProxy() const {
					return proxy;
				}

				std::unordered_map<DistributedId, int> balance(
						std::unordered_map<DistributedId, NodeType*> nodes,
						std::unordered_map<DistributedId, int> fixedVertices
						) override;

		};

	template<typename NodeType> std::unordered_map<DistributedId, int>
		ZoltanLoadBalancing<NodeType>::balance(
			std::unordered_map<DistributedId, NodeType*> nodes,
			std::unordered_map<DistributedId, int> fixedVertices
			) {
			this->nodes = nodes;
			this->fixedVertices = fixedVertices;
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
			int err = this->zoltan.LB_Partition(
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

			std::unordered_map<DistributedId, int> partition;
			for(int i = 0; i < export_node_num; i++) {
				partition[zoltan::utils::read_zoltan_id(&export_node_global_ids[i * num_gid_entries])]
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

			this->zoltan.Set_Param("LB_APPROACH", "REPARTITION");
			return partition;
	}

	/*
	 * Initializes zoltan parameters and zoltan lb query functions.
	 */
	template<typename NodeType> void ZoltanLoadBalancing<NodeType>::setUpZoltan() {
		FPMAS::config::zoltan_config(&this->zoltan);

		// Initializes Zoltan Node Load Balancing functions
		this->zoltan.Set_Num_Fixed_Obj_Fn(zoltan::num_fixed_obj_fn<NodeType>, &this->fixedVertices);
		this->zoltan.Set_Fixed_Obj_List_Fn(zoltan::fixed_obj_list_fn<NodeType>, &this->fixedVertices);
		this->zoltan.Set_Num_Obj_Fn(zoltan::num_obj<NodeType>, &this->nodes);
		this->zoltan.Set_Obj_List_Fn(zoltan::obj_list<NodeType>, &this->nodes);
		this->zoltan.Set_Num_Edges_Multi_Fn(zoltan::num_edges_multi_fn<NodeType>, &this->nodes);
		this->zoltan.Set_Edge_List_Multi_Fn(zoltan::edge_list_multi_fn<NodeType>, &this->nodesProxyPack);
	}
}
#endif
