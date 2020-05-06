#ifndef MOCK_REQUEST_HANDLER_H
#define MOCK_REQUEST_HANDLER_H
#include "gmock/gmock.h"

#include "../mock_mutex.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

using FPMAS::api::graph::parallel::synchro::hard::Request;

template<typename T>
class MockHardSyncMutex :
	public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>,
	public MockMutex<T> {
		public:
			MOCK_METHOD(void, pushRequest, (Request), (override));

			MOCK_METHOD(std::queue<Request>, requestsToProcess, (), (override));
	};

#endif
