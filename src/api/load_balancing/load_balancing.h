#ifndef LOAD_BALANCING_API_H
#define LOAD_BALANCING_API_H

#include <set>
#include <unordered_map>

#include "api/graph/parallel/distributed_node.h"
#include "graph/parallel/distributed_id.h"
#include "api/graph/base/id.h"

namespace fpmas::api::load_balancing {
	template<typename T>
		class FixedVerticesLoadBalancing {
			protected:
				typedef api::graph::parallel::DistributedNode<T> NodeType;
				typedef api::graph::base::IdHash<DistributedId> NodeIdHash;

			public:
				typedef std::unordered_map<DistributedId, int, NodeIdHash> PartitionMap;
				typedef std::unordered_map<
					DistributedId, const NodeType*, NodeIdHash
					> ConstNodeMap;
				virtual PartitionMap balance(
						ConstNodeMap nodes,
						PartitionMap fixedVertices
						) = 0;
		};

	template<typename T>
		class LoadBalancing {
			protected:
				typedef api::graph::parallel::DistributedNode<T> NodeType;
				typedef api::graph::base::IdHash<DistributedId> NodeIdHash;

			public:
				typedef std::unordered_map<DistributedId, int, NodeIdHash> PartitionMap;
				typedef std::unordered_map<
					DistributedId, const NodeType*, NodeIdHash
					> ConstNodeMap;
				virtual PartitionMap balance(ConstNodeMap nodes) = 0;
		};

}
#endif
