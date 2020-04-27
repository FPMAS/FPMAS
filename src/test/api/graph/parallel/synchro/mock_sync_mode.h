#ifndef MOCK_SYNC_MODE_H
#define MOCK_SYNC_MODE_H

#include "api/graph/parallel/synchro/sync_mode.h"
#include "mock_mutex.h"

template<typename NodeType, typename ArcType>
class MockDataSync : public FPMAS::api::graph::parallel::synchro::DataSync<NodeType, ArcType> {
	public:
		MOCK_METHOD(void, synchronize, (
				(FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>)&
				), (override));

};

template<typename NodeType, typename ArcType>
class MockSyncLinker : public FPMAS::api::graph::parallel::synchro::SyncLinker<NodeType, ArcType> {
	public:
		MOCK_METHOD(void, link, (const ArcType*), (override));
		MOCK_METHOD(void, unlink, (const ArcType*), (override));
		MOCK_METHOD(void, synchronize, (
				(FPMAS::api::graph::parallel::DistributedGraph<NodeType, ArcType>)&
				), (override));

};

template<
	template<typename> class Mutex = MockMutex,
	template<typename, typename> class SyncLinker = MockSyncLinker,
	template<typename, typename> class DataSync = MockDataSync>
class MockSyncMode : public FPMAS::api::graph::parallel::synchro::SyncMode<Mutex, SyncLinker, DataSync> {
	/*
	 *public:
	 *    MOCK_METHOD(void, synchronize, (
	 *        FPMAS::api::communication::MpiCommunicator&,
	 *        (FPMS::api::graph::parallel::DistributedGraph<NodeType, ArcType>&)
	 *    ), (override));
	 */

};

#endif
