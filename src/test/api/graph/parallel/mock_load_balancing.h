#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "api/graph/parallel/load_balancing.h"

template<typename NodeType>
class MockLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<NodeType> {
	typedef FPMAS::api::graph::parallel::LoadBalancing<NodeType> LBbase;
	using typename LBbase::node_map;
	using typename LBbase::partition;
	public:
		MOCK_METHOD(
			partition,
			balance,
			(node_map, partition),
			(override)
			);

};

#endif
