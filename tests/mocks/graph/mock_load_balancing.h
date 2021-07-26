#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "fpmas/api/graph/load_balancing.h"

template<typename T>
class MockFixedVerticesLoadBalancing : public fpmas::api::graph::FixedVerticesLoadBalancing<T> {
	public:
		MOCK_METHOD(
			fpmas::api::graph::PartitionMap,
			balance,
			(fpmas::api::graph::NodeMap<T>, fpmas::api::graph::PartitionMap),
			(override)
			);

		MOCK_METHOD(
			fpmas::api::graph::PartitionMap,
			balance,
			(fpmas::api::graph::NodeMap<T>, fpmas::api::graph::PartitionMap, fpmas::api::graph::PartitionMode),
			(override)
			);
};

template<typename T>
class MockLoadBalancing : public fpmas::api::graph::LoadBalancing<T> {
	public:
		MOCK_METHOD(
			fpmas::api::graph::PartitionMap,
			balance,
			(fpmas::api::graph::NodeMap<T>),
			(override)
			);

		MOCK_METHOD(
			fpmas::api::graph::PartitionMap,
			balance,
			(fpmas::api::graph::NodeMap<T>, fpmas::api::graph::PartitionMode),
			(override)
			);
};


#endif
