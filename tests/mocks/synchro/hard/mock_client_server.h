#ifndef MOCK_TERMINATION_H
#define MOCK_TERMINATION_H
#include "gmock/gmock.h"

#include "fpmas/synchro/hard/api/client_server.h"

using fpmas::synchro::hard::api::Epoch;

class MockTerminationAlgorithm : public fpmas::synchro::hard::api::TerminationAlgorithm {
	public:
		template<typename... Args>
			MockTerminationAlgorithm(Args&...) {}

		MOCK_METHOD(
			void, terminate,
			(fpmas::synchro::hard::api::Server&),
			(override));

};

template<typename T>
class MockMutexClient
: public fpmas::synchro::hard::api::MutexClient<T> {
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
: public fpmas::synchro::hard::api::MutexServer<T> {
	public:
		using fpmas::synchro::hard::api::MutexServer<T>::lock;
		using fpmas::synchro::hard::api::MutexServer<T>::lockShared;

		MockMutexServer() {
			EXPECT_CALL(*this, setEpoch).Times(::testing::AnyNumber());
		}

		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, manage,
				(DistributedId, fpmas::synchro::hard::api::HardSyncMutex<T>*),
				(override));
		MOCK_METHOD(void, remove, (DistributedId), (override));

		MOCK_METHOD(void, wait,
				(const fpmas::synchro::hard::api::MutexRequest&),
				(override));
		MOCK_METHOD(void, notify, (DistributedId), (override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));
};

template<typename T>
class MockLinkClient
: public fpmas::synchro::hard::api::LinkClient<T> {
	typedef fpmas::api::graph::DistributedEdge<T> EdgeApi;
	typedef fpmas::api::graph::DistributedNode<T> NodeApi;
	public:
		MOCK_METHOD(void, link, (const EdgeApi*), (override));
		MOCK_METHOD(void, unlink, (const EdgeApi*), (override));
		MOCK_METHOD(void, removeNode, (const NodeApi*), (override));

};

class MockLinkServer
: public fpmas::synchro::hard::api::LinkServer {
	public:
		MockLinkServer() {
			EXPECT_CALL(*this, setEpoch).Times(::testing::AnyNumber());
		}

		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));

		MOCK_METHOD(void, lockUnlink, (DistributedId), (override));
		MOCK_METHOD(bool, isLockedUnlink, (DistributedId), (override));
		MOCK_METHOD(void, unlockUnlink, (DistributedId), (override));
};
#endif
