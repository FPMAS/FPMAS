#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "api/graph/parallel/load_balancing.h"

template<typename NodeType>
class MockLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<NodeType> {
	typedef FPMAS::api::graph::parallel::LoadBalancing<NodeType> base;
	using typename base::node_map;
	using typename base::partition_type;
	public:
		MOCK_METHOD(
			partition_type,
			balance,
			(node_map, partition_type),
			(override)
			);

};

#endif
