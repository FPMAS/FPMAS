#ifndef MOCK_LOAD_BALANCING_H
#define MOCK_LOAD_BALANCING_H

#include "gmock/gmock.h"

#include "fpmas/api/load_balancing/load_balancing.h"

template<typename T>
class MockFixedVerticesLoadBalancing : public fpmas::api::load_balancing::FixedVerticesLoadBalancing<T> {
	typedef fpmas::api::load_balancing::LoadBalancing<T> Base;
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

template<typename T>
class MockLoadBalancing : public fpmas::api::load_balancing::LoadBalancing<T> {
	typedef fpmas::api::load_balancing::LoadBalancing<T> Base;
	public:
		using typename Base::ConstNodeMap;
		using typename Base::PartitionMap;
		MOCK_METHOD(
			PartitionMap,
			balance,
			(ConstNodeMap),
			(override)
			);

};


#endif
