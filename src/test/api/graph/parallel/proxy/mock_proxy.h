#ifndef MOCK_PROXY_H
#define MOCK_PROXY_H
#include "gmock/gmock.h"

#include "api/graph/parallel/proxy/proxy.h"
#include "graph/parallel/distributed_id.h"

class MockProxy : public FPMAS::api::graph::parallel::Proxy<DistributedId> {
	public:
		MOCK_METHOD(void, setOrigin, (DistributedId, int), (override));
		MOCK_METHOD(int, getOrigin, (DistributedId), (const, override));

		MOCK_METHOD(void, setCurrentLocation, (DistributedId, int), (override));
		MOCK_METHOD(int, getCurrentLocation, (DistributedId), (const, override));

		MOCK_METHOD(void, synchronize, (), (override));
};
#endif
