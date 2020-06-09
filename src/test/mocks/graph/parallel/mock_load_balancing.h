#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "api/graph/parallel/load_balancing.h"

template<typename T>
class MockLoadBalancing : public FPMAS::api::graph::parallel::LoadBalancing<T> {
	typedef FPMAS::api::graph::parallel::LoadBalancing<T> Base;
	public:
		using typename Base::ConstNodeMap;
		using typename Base::PartitionMap;
		MOCK_METHOD(
			PartitionMap,
			balance,
			(ConstNodeMap, PartitionMap),
			(override)
			);

};

#endif
