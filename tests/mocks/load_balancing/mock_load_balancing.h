#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "fpmas/api/load_balancing/load_balancing.h"

template<typename T>
class MockFixedVerticesLoadBalancing : public fpmas::api::load_balancing::FixedVerticesLoadBalancing<T> {
	public:
		MOCK_METHOD(
			fpmas::api::load_balancing::PartitionMap,
			balance,
			(fpmas::api::load_balancing::NodeMap<T>, fpmas::api::load_balancing::PartitionMap),
			(override)
			);

};

template<typename T>
class MockLoadBalancing : public fpmas::api::load_balancing::LoadBalancing<T> {
	public:
		MOCK_METHOD(
			fpmas::api::load_balancing::PartitionMap,
			balance,
			(fpmas::api::load_balancing::NodeMap<T>),
			(override)
			);

};


#endif
