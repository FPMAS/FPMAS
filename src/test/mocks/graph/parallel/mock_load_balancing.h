#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "api/graph/parallel/load_balancing.h"

template<typename NodeType>
class MockLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<NodeType> {
	typedef FPMAS::api::graph::parallel::LoadBalancing<NodeType> Base;
	using typename Base::NodeMap;
	public:
		using typename Base::PartitionMap;
		MOCK_METHOD(
			PartitionMap,
			balance,
			(NodeMap, PartitionMap),
			(override)
			);

};

#endif
