#ifndef MOCK_REQUEST_HANDLER_H
#define MOCK_REQUEST_HANDLER_H
#include "gmock/gmock.h"

#include "../mock_mutex.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

using FPMAS::api::graph::parallel::synchro::hard::MutexRequest;

template<typename T>
class MockHardSyncMutex :
	public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>,
	public MockMutex<T> {
		public:
			MOCK_METHOD(void, pushRequest, (MutexRequest), (override));

			MOCK_METHOD(std::queue<MutexRequest>, requestsToProcess, (), (override));
	};

#endif
