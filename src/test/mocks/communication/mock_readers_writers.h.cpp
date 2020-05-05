#ifndef READERS_WRITERS_MOCKS_H
#define READERS_WRITERS_MOCKS_H
#include "gmock/gmock.h"

#include "api/communication/readers_writers.h"

#include "graph/parallel/distributed_id.h"

using FPMAS::graph::parallel::DistributedId;

class MockReadersWriters : public FPMAS::api::communication::ReadersWriters {
	public:
		MOCK_METHOD(void, read, (int), (override));
		MOCK_METHOD(void, acquire, (int), (override));
		MOCK_METHOD(void, release, (), (override));
		MOCK_METHOD(void, lock, (), (override));
		MOCK_METHOD(bool, isLocked, (), (const override));
};

class MockReadersWritersManager : public FPMAS::api::communication::ReadersWritersManager<MockReadersWriters> {
	public:
		MockReadersWritersManager(FPMAS::api::communication::RequestHandler& requestHandler, int rank)
			: FPMAS::api::communication::ReadersWritersManager<MockReadersWriters>(requestHandler) {}

		MOCK_METHOD(MockReadersWriters&, AccessOp, (const DistributedId &), ());
		MOCK_METHOD(void, clear, (), (override));

		MockReadersWriters& operator[](const DistributedId& id) override { return AccessOp(id);}
};

#endif
