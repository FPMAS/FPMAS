#ifndef LOAD_BALANCING_API_H
#define LOAD_BALANCING_API_H

#include <set>
#include <unordered_map>

#include "graph/parallel/distributed_id.h"
#include "api/graph/base/id.h"

namespace FPMAS::api::graph::parallel {
	template<typename NodeType>
		class LoadBalancing {
			protected:
				typedef FPMAS::api::graph::base::IdHash<DistributedId> node_id_hash;

			public:
				typedef std::unordered_map<DistributedId, int, node_id_hash> partition;
				typedef std::unordered_map<
					DistributedId, NodeType*, node_id_hash
					> node_map;
				virtual partition balance(
						node_map nodes,
						partition fixedVertices
						) = 0;
		};
}
#endif
