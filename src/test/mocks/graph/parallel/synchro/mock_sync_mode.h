#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "mock_mutex.h"

class MockDataSync : public FPMAS::api::graph::parallel::synchro::DataSync {
	public:
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename ArcType>
class MockSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<ArcType> {
	public:
		MockSyncLinker(FPMAS::api::communication::MpiCommunicator&) {}

		MOCK_METHOD(void, link, (const ArcType*), (override));
		MOCK_METHOD(void, unlink, (const ArcType*), (override));
		MOCK_METHOD(void, synchronize, (), (override));

};

template<typename Node, typename Arc, typename Mutex>
class MockSyncModeRuntime : public FPMAS::api::graph::parallel::synchro::SyncModeRuntime<Node, Arc, Mutex> {
	public:
		MockSyncModeRuntime(
			FPMAS::api::graph::parallel::DistributedGraph<Node, Arc>&,
			FPMAS::api::communication::MpiCommunicator&) {}

		MOCK_METHOD(void, setUp, (DistributedId, Mutex&), (override));
		MOCK_METHOD(FPMAS::api::graph::parallel::synchro::SyncLinker<Arc>&, getSyncLinker, (), (override));
		MOCK_METHOD(FPMAS::api::graph::parallel::synchro::DataSync&, getDataSync, (), (override));

};
template<
	template<typename> class Mutex = MockMutex,
	template<typename, typename, typename> class Runtime = MockSyncModeRuntime>
class MockSyncMode : public FPMAS::api::graph::parallel::synchro::SyncMode<Mutex, Runtime> {

};

#endif
