#ifndef LOAD_BALANCING_API_H
#define LOAD_BALANCING_API_H

#include <set>
#include <unordered_map>

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {
	template<typename NodeType>
		class LoadBalancing {
			public:
				virtual std::unordered_map<DistributedId, int> balance(
						std::unordered_map<DistributedId, NodeType*> nodes,
						std::unordered_map<DistributedId, int> fixedVertices
						) = 0;
		};
}
#endif
