#ifndef MOCK_REQUEST_HANDLER_H
#define MOCK_REQUEST_HANDLER_H
#include "gmock/gmock.h"

#include "../mock_mutex.h"
#include "../mock_sync_mode.h"
#include "fpmas/synchro/hard/api/hard_sync_mode.h"

using fpmas::synchro::hard::api::MutexRequest;

template<typename T>
class MockHardSyncMutex :
	public fpmas::synchro::hard::api::HardSyncMutex<T>,
	public MockMutex<T> {
		public:
			MOCK_METHOD(void, pushRequest, (MutexRequest), (override));

			MOCK_METHOD(std::queue<MutexRequest>, requestsToProcess, (), (override));
	};

template<typename T>
class MockHardSyncLinker :
	public fpmas::synchro::hard::api::HardSyncLinker<T>,
	public MockSyncLinker<T> {
		public:
			MOCK_METHOD(void, registerNodeToRemove, (fpmas::api::graph::DistributedNode<T>*), (override));
	};

#endif
