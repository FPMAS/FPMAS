#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/distributed_node.h"

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename Implementation, typename DistributedNodeType, typename DistributedArcType>
	class DistributedGraph 
		: public virtual FPMAS::api::graph::base::Graph<Implementation, DistributedNodeType, DistributedArcType> {
		public:
			typedef FPMAS::api::graph::base::Graph<Implementation, DistributedNodeType, DistributedArcType> graph_base;
			using typename graph_base::node_type;
			using typename graph_base::arc_type;
			using typename graph_base::layer_id_type;
			using typename graph_base::node_map;
			using typename graph_base::arc_map;

			typedef typename LoadBalancing<DistributedNodeType>::partition_type partition_type;

		public:

			virtual const node_map& getLocalNodes() const = 0;

			virtual void balance() = 0;

			virtual void distribute(partition_type partition) = 0;

			virtual void synchronize() = 0;
	};
}

#endif
