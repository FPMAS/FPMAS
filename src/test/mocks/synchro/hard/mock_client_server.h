#ifndef MOCK_TERMINATION_H
#define MOCK_TERMINATION_H
#include "gmock/gmock.h"

#include "api/synchro/hard/client_server.h"

using FPMAS::api::synchro::hard::Epoch;

class MockTerminationAlgorithm : public FPMAS::api::synchro::hard::TerminationAlgorithm {
	public:
		template<typename... Args>
			MockTerminationAlgorithm(Args&...) {}

		MOCK_METHOD(
			void, terminate,
			(FPMAS::api::synchro::hard::Server&),
			(override));

};

template<typename T>
class MockMutexClient
: public FPMAS::api::synchro::hard::MutexClient<T> {
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
: public FPMAS::api::synchro::hard::MutexServer<T> {
	public:
		using FPMAS::api::synchro::hard::MutexServer<T>::lock;
		using FPMAS::api::synchro::hard::MutexServer<T>::lockShared;

		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, manage,
				(DistributedId, FPMAS::api::synchro::hard::HardSyncMutex<T>*),
				(override));
		MOCK_METHOD(void, remove, (DistributedId), (override));

		MOCK_METHOD(void, wait,
				(const FPMAS::api::synchro::hard::MutexRequest&),
				(override));
		MOCK_METHOD(void, notify, (DistributedId), (override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));
};

template<typename T>
class MockLinkClient
: public FPMAS::api::synchro::hard::LinkClient<T> {
	typedef FPMAS::api::graph::parallel::DistributedArc<T> ArcApi;
	public:
		MOCK_METHOD(void, link, (const ArcApi*), (override));
		MOCK_METHOD(void, unlink, (const ArcApi*), (override));

};

class MockLinkServer
: public FPMAS::api::synchro::hard::LinkServer {
	public:
		MOCK_METHOD(void, setEpoch, (Epoch), (override));
		MOCK_METHOD(Epoch, getEpoch, (), (const, override));

		MOCK_METHOD(void, handleIncomingRequests, (), (override));
};
#endif
