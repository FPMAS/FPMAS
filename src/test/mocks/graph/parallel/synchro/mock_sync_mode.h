#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "mock_mutex.h"

using ::testing::ReturnNew;

class MockDataSync : public FPMAS::api::graph::parallel::synchro::DataSync {
	public:
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<T> {
	public:
		MOCK_METHOD(void, link, (const FPMAS::api::graph::parallel::DistributedArc<T>*), (override));
		MOCK_METHOD(void, unlink, (const FPMAS::api::graph::parallel::DistributedArc<T>*), (override));
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename T>
class MockSyncModeRuntime : public FPMAS::api::graph::parallel::synchro::SyncModeRuntime<T> {
	public:
		MockSyncModeRuntime(
			FPMAS::api::graph::parallel::DistributedGraph<T>&,
			FPMAS::api::communication::MpiCommunicator&) {
			ON_CALL(*this, buildMutex).WillByDefault(ReturnNew<MockMutex<int>>());
		}

		MOCK_METHOD(MockMutex<T>*, buildMutex, (DistributedId, T&), (override));
		MOCK_METHOD(FPMAS::api::graph::parallel::synchro::SyncLinker<T>&, getSyncLinker, (), (override));
		MOCK_METHOD(FPMAS::api::graph::parallel::synchro::DataSync&, getDataSync, (), (override));

};

template<
	template<typename> class Mutex = MockMutex,
	template<typename> class Runtime = MockSyncModeRuntime>
using MockSyncMode = FPMAS::api::graph::parallel::synchro::SyncMode<Mutex, Runtime>;
#endif
