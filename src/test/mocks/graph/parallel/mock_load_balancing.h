#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "api/graph/parallel/load_balancing.h"

template<typename T>
class MockLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<T> {
	typedef FPMAS::api::graph::parallel::LoadBalancing<T> Base;
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
