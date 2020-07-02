#ifndef MOCK_TERMINATION_H
#define MOCK_TERMINATION_H
#include "gmock/gmock.h"

#include "fpmas/api/synchro/hard/client_server.h"

using fpmas::api::synchro::hard::Epoch;

class MockTerminationAlgorithm : public fpmas::api::synchro::hard::TerminationAlgorithm {
	public:
		template<typename... Args>
			MockTerminationAlgorithm(Args&...) {}

		MOCK_METHOD(
			void, terminate,
			(fpmas::api::synchro::hard::Server&),
			(override));

};

template<typename T>
class MockMutexClient
: public fpmas::api::synchro::hard::MutexClient<T> {
	public:
		MOCK_METHOD(T, read, (DistributedId, int), (override));
		MOCK_METHOD(void, releaseRead, (DistributedId, int), (override));

		MOCK_METHOD(T, acquire, (DistributedId, int), (override));
		MOCK_METHOD(void, releaseAcquire, (DistributedId, const T&, int), (override));

		MOCK_METHOD(void, lock, (DistributedId, int), (override));
		MOCK_METHOD(void, unlock, (DistributedId, int), (override));

		MOCK_METHOD(void, lockShared, (DistributedId, int), (override));
		MOCK_METHOD(void, unlockShared, (DistributedId, int), (override));
};

template<typename T>
class MockMutexServer
: public fpmas::api::synchro::hard::MutexServer<T> {
	public:
		using fpmas::api::synchro::hard::MutexServer<T>::lock;
		using fpmas::api::synchro::hard::MutexServer<T>::lockShared;

		MockMutexServer() {
			EXPECT_CALL(*this, setEpoch).Times(::testing::AnyNumber());
		}

		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, manage,
				(DistributedId, fpmas::api::synchro::hard::HardSyncMutex<T>*),
				(override));
		MOCK_METHOD(void, remove, (DistributedId), (override));

		MOCK_METHOD(void, wait,
				(const fpmas::api::synchro::hard::MutexRequest&),
				(override));
		MOCK_METHOD(void, notify, (DistributedId), (override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));
};

template<typename T>
class MockLinkClient
: public fpmas::api::synchro::hard::LinkClient<T> {
	typedef fpmas::api::graph::parallel::DistributedArc<T> ArcApi;
	public:
		MOCK_METHOD(void, link, (const ArcApi*), (override));
		MOCK_METHOD(void, unlink, (const ArcApi*), (override));

};

class MockLinkServer
: public fpmas::api::synchro::hard::LinkServer {
	public:
		MockLinkServer() {
			EXPECT_CALL(*this, setEpoch).Times(::testing::AnyNumber());
		}

		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));
};
#endif
