#ifndef READERS_WRITERS_MOCKS_H
#define READERS_WRITERS_MOCKS_H
#include "gmock/gmock.h"

#include "fpmas/api/communication/readers_writers.h"

#include "graph/parallel/distributed_id.h"

using fpmas::graph::parallel::DistributedId;

class MockReadersWriters : public fpmas::api::communication::ReadersWriters {
	public:
		MOCK_METHOD(void, read, (int), (override));
		MOCK_METHOD(void, acquire, (int), (override));
		MOCK_METHOD(void, release, (), (override));
		MOCK_METHOD(void, lock, (), (override));
		MOCK_METHOD(bool, isLocked, (), (const override));
};

class MockReadersWritersManager : public fpmas::api::communication::ReadersWritersManager<MockReadersWriters> {
	public:
		MockReadersWritersManager(fpmas::api::communication::RequestHandler& requestHandler, int rank)
			: fpmas::api::communication::ReadersWritersManager<MockReadersWriters>(requestHandler) {}

		MOCK_METHOD(MockReadersWriters&, AccessOp, (const DistributedId &), ());
		MOCK_METHOD(void, clear, (), (override));

		MockReadersWriters& operator[](const DistributedId& id) override { return AccessOp(id);}
};

#endif
