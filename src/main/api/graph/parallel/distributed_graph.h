#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

#include "api/communication/communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/distributed_node.h"

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename DistributedNodeType, typename DistributedArcType>
	class DistributedGraph 
		: public virtual FPMAS::api::graph::base::Graph<DistributedNodeType, DistributedArcType> {
		public:
			typedef FPMAS::api::graph::base::Graph<DistributedNodeType, DistributedArcType> graph_base;
			using typename graph_base::node_type;
			using typename graph_base::arc_type;
			using typename graph_base::layer_id_type;
			using typename graph_base::node_map;
			using typename graph_base::arc_map;

			typedef typename LoadBalancing<DistributedNodeType>::partition_type partition_type;

		public:
			virtual FPMAS::api::communication::MpiCommunicator& getMpiCommunicator() = 0;
			virtual const FPMAS::api::communication::MpiCommunicator& getMpiCommunicator() const = 0;

			virtual node_type* importNode(const node_type&) = 0;
			virtual arc_type* importArc(const arc_type&) = 0;
			virtual void clear(node_type*) = 0;

			virtual void balance() = 0;
			virtual void distribute(partition_type partition) = 0;
			virtual void synchronize() = 0;
	};
}

#endif
