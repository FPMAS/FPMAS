#ifndef MOCK_REQUEST_HANDLER_H
#define MOCK_REQUEST_HANDLER_H
#include "gmock/gmock.h"

#include "../mock_mutex.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

using FPMAS::api::graph::parallel::synchro::hard::Epoch;
using FPMAS::api::graph::parallel::synchro::hard::Request;

template<typename T>
class MockHardSyncMutex :
	public FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>,
	public MockMutex<T> {
		public:
			MOCK_METHOD(void, pushRequest, (Request), (override));

			MOCK_METHOD(std::queue<Request>, requestsToProcess, (), (override));
	};

template<typename T>
class MockMutexClient
	: public FPMAS::api::graph::parallel::synchro::hard::MutexClient<T> {
		public:
			MOCK_METHOD(void, manage,
					(DistributedId, FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>*),
					(override));
			MOCK_METHOD(void, remove, (DistributedId), (override));
			MOCK_METHOD(T, read, (DistributedId, int), (override));
			MOCK_METHOD(T, acquire, (DistributedId, int), (override));
			MOCK_METHOD(void, release, (DistributedId, int), (override));

			MOCK_METHOD(void, lock, (DistributedId, int), (override));
			MOCK_METHOD(void, unlock, (DistributedId, int), (override));
	};

template<typename T>
class MockMutexServer
	: public FPMAS::api::graph::parallel::synchro::hard::MutexServer<T> {
		public:
			MOCK_METHOD(void, setEpoch, (Epoch), (override));
			MOCK_METHOD(Epoch, getEpoch, (), (const, override));

			MOCK_METHOD(void, manage,
					(DistributedId, FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>*),
					(override));
			MOCK_METHOD(void, remove, (DistributedId), (override));

			MOCK_METHOD(void, wait,
				(const FPMAS::api::graph::parallel::synchro::hard::Request&),
				(override));
			MOCK_METHOD(void, notify, (DistributedId), (override));

			MOCK_METHOD(void, handleIncomingRequests, (), (override));
	};

#endif
