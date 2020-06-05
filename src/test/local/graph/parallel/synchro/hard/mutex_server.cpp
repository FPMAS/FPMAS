/* INDEX */
/*
 * # MutexServerTest
 * ## mutex_server_test_manage
 * ## mutex_server_test_remove
 *
 * # MutexServerHandleIncomingRequestsTest
 * ## mutex_server_handle_incoming_requests_test_unlocked_read
 * ## mutex_server_handle_incoming_requests_test_locked_read
 * ## mutex_server_handle_incoming_requests_test_locked_shared_read
 *
 * ## mutex_server_handle_incoming_requests_test_unlocked_acquire
 * ## mutex_server_handle_incoming_requests_test_locked_acquire
 * ## mutex_server_handle_incoming_requests_test_locked_shared_acquire
 *
 * ## mutex_server_handle_incoming_requests_test_release_no_waiting_queue
 * ## mutex_server_handle_incoming_requests_test_release_with_waiting_queue
 * ## mutex_server_handle_incoming_requests_test_release_with_pending_lock
 * ## mutex_server_handle_incoming_requests_test_release_with_pending_acquire
 *
 * ## mutex_server_handle_incoming_requests_test_unlocked_lock
 * ## mutex_server_handle_incoming_requests_test_locked_lock
 * ## mutex_server_handle_incoming_requests_test_locked_shared_lock
 *
 * ## mutex_server_handle_incoming_requests_test_unlocked_lock_shared
 * ## mutex_server_handle_incoming_requests_test_locked_lock_shared
 * ## mutex_server_handle_incoming_requests_test_locked_shared_lock_shared
 *
 * ## mutex_server_handle_incoming_requests_test_unlock_no_waiting_queues
 * ## mutex_server_handle_incoming_requests_test_unlock_with_waiting_queue
 * ## mutex_server_handle_incoming_requests_test_unlock_with_pending_lock
 * ## mutex_server_handle_incoming_requests_test_unlock_with_pending_acquire
 *
 * ## mutex_server_handle_incoming_requests_test_partial_unlock_shared
 * ## mutex_server_handle_incoming_requests_test_last_unlock_shared_no_waiting_queue
 * ## mutex_server_handle_incoming_requests_test_last_unlock_shared_with_waiting_queue
 * ## mutex_server_handle_incoming_requests_test_last_unlock_shared_with_pending_lock
 * ## mutex_server_handle_incoming_requests_test_last_unlock_shared_with_pending_acquire
 *
 * ## mutex_server_handle_incoming_requests_all
 */
#include "graph/parallel/synchro/hard/mutex_server.h"

#include "../mocks/communication/mock_communication.h"
#include "api/graph/parallel/synchro/hard/enums.h"
#include "../mocks/graph/parallel/synchro/hard/mock_hard_sync_mutex.h"

using ::testing::_;
using ::testing::A;
using ::testing::An;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Pair;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Pointee;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

using FPMAS::graph::parallel::synchro::hard::Epoch;
using FPMAS::graph::parallel::synchro::hard::Tag;
using FPMAS::graph::parallel::synchro::hard::MutexServer;
using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;
using FPMAS::graph::parallel::synchro::hard::MutexRequestType;

/*******************/
/* MutexServerTest */
/*******************/
class MutexServerTest : public ::testing::Test {
	protected:
	MockMpiCommunicator<3, 8> comm;
	MockMpi<DistributedId> id_mpi {comm};
	MockMpi<int> data_mpi {comm};
	MockMpi<DataUpdatePack<int>> data_update_mpi {comm};

	MutexServer<int> server {comm, id_mpi, data_mpi, data_update_mpi};
};

/*
 * mutex_server_test_manage
 */
TEST_F(MutexServerTest, manage) {
	MockHardSyncMutex<int> mock_mutex;
	server.manage(DistributedId(5, 4), &mock_mutex);
	server.manage(DistributedId(0, 2), &mock_mutex);

	ASSERT_THAT(server.getManagedMutexes(), UnorderedElementsAre(
		Pair(DistributedId(5, 4), _),
		Pair(DistributedId(0, 2), _)
	));
}

TEST_F(MutexServerTest, remove) {
	MockHardSyncMutex<int> mock_mutex;
	server.manage(DistributedId(5, 4), &mock_mutex);
	server.manage(DistributedId(0, 2), &mock_mutex);

	server.remove(DistributedId(5, 4));

	ASSERT_THAT(server.getManagedMutexes(), ElementsAre(
		Pair(DistributedId(0, 2), _)
	));
}

/*****************************************/
/* MutexServerHandleIncomingRequestsTest */
/*****************************************/
class MutexServerHandleIncomingRequestsTest : public MutexServerTest {
	private:
		class MockProbe {
			private:
				int source;
			public:
				MockProbe(int source) : source(source) {}
				void operator()(int, int tag, MPI_Status* status) {
					status->MPI_SOURCE = this->source;
					status->MPI_TAG = tag;
				}
		};
		std::vector<MockHardSyncMutex<int>*> mocks;


	protected:
		void SetUp() override {
			ON_CALL(comm, Iprobe)
				.WillByDefault(Return(false));
			// By default, expect any probe and do default actions
			EXPECT_CALL(comm, Iprobe).Times(AnyNumber());
		}

		MockHardSyncMutex<int>* mock_mutex(DistributedId id, int& data) {
			auto* mock = new MockHardSyncMutex<int>();
			mocks.push_back(mock);
			server.manage(id, mock);

			EXPECT_CALL(*mock, data()).WillRepeatedly(ReturnRef(data));
			return mock;
		}

		void expectRead(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::READ, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::READ)))
					))
				.WillOnce(Return(id));
		}

		void expectReadResponse(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(*mock, _lockShared);
			EXPECT_CALL(data_mpi,
				send(Ref(mock->data()), source, Epoch::EVEN | Tag::READ_RESPONSE));
		}
	
		void expectUnlockedRead(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectRead(source, id, mock);
			
			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));

			expectReadResponse(source, id, mock);
		}

		void expectLockedRead(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectRead(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(true));
			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::READ)));
		}

		void expectLockedSharedRead(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectUnlockedRead(source, id, mock);
		}

		void expectAcquire(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::ACQUIRE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::ACQUIRE)))
					))
				.WillOnce(Return(id));
		}

		void expectAcquireResponse(int source, DistributedId id, int updatedData, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(data_mpi,
				send(updatedData, source, Epoch::EVEN | Tag::ACQUIRE_RESPONSE));
			EXPECT_CALL(*mock, _lock);
		}

		void expectUnlockedAcquire(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectAcquire(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));
			EXPECT_CALL(*mock, lockedShared).WillRepeatedly(Return(0));

			expectAcquireResponse(source, id, mock->data(), mock);
		}

		void expectLockedAcquire(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectAcquire(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(true));
			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::ACQUIRE)));
		}

		void expectLockedSharedAcquire(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectAcquire(source, id, mock);

			// The resource is not locked...
			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));
			// ... but some threads still have a shared lock
			EXPECT_CALL(*mock, lockedShared).WillRepeatedly(Return(5));

			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::ACQUIRE)));
		}

		void expectRelease(
			int source, DistributedId id, int updatedData,
			std::queue<MutexRequest>& requests, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(*mock, requestsToProcess)
				.WillOnce(Return(requests));

			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::RELEASE_ACQUIRE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(data_update_mpi,
				recv(Pointee(AllOf(
					Field(&MPI_Status::MPI_SOURCE, source),
					Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::RELEASE_ACQUIRE))))
				)
				.WillOnce(Return(DataUpdatePack(id, updatedData)));

			EXPECT_CALL(*mock, _unlock);
		}

		void expectLock(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::LOCK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::LOCK)))
					))
				.WillOnce(Return(id));
		}

		void expectLockResponse(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, send(source, Epoch::EVEN | Tag::LOCK_RESPONSE));
			EXPECT_CALL(*mock, _lock);
		}

		void expectUnlockedLock(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectLock(source, id, mock);
			
			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));
			EXPECT_CALL(*mock, lockedShared).WillRepeatedly(Return(0));

			expectLockResponse(source, id, mock);
		}

		void expectLockedLock(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectLock(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(true));

			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::LOCK)));
		}

		void expectLockedSharedLock(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectLock(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));
			EXPECT_CALL(*mock, lockedShared).WillRepeatedly(Return(5));

			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::LOCK)));
		}

		void expectUnlock(
			int source, DistributedId id,
			std::queue<MutexRequest>& requests, MockHardSyncMutex<int>* mock) {

			EXPECT_CALL(*mock, requestsToProcess)
				.WillOnce(Return(requests));

			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::UNLOCK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi,
				recv(Pointee(AllOf(
					Field(&MPI_Status::MPI_SOURCE, source),
					Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::UNLOCK)
					)))
				)
				.WillOnce(Return(id));

			EXPECT_CALL(*mock, _unlock);
		}

		void expectLockShared(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::LOCK_SHARED, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::LOCK_SHARED)))
					))
				.WillOnce(Return(id));
		}

		void expectLockSharedResponse(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(*mock, _lockShared);
			EXPECT_CALL(comm, send(source, Epoch::EVEN | Tag::LOCK_SHARED_RESPONSE));
		}

		void expectUnlockedLockShared(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectLockShared(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));

			expectLockSharedResponse(source, id, mock);
		}

		void expectLockedLockShared(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectLockShared(source, id, mock);

			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(true));

			EXPECT_CALL(*mock, pushRequest(MutexRequest(id, source, MutexRequestType::LOCK_SHARED)));
		}

		void expectLockedSharedLockShared(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			expectUnlockedLockShared(source, id, mock);
		}

		void expectUnlockShared(
			int source, DistributedId id, int shared_locks,
			std::queue<MutexRequest>& requests, MockHardSyncMutex<int>* mock) {

			EXPECT_CALL(*mock, lockedShared).WillRepeatedly(Return(shared_locks));

			EXPECT_CALL(*mock, requestsToProcess)
				.WillOnce(Return(requests));

			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::UNLOCK_SHARED, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi,
				recv(Pointee(AllOf(
					Field(&MPI_Status::MPI_SOURCE, source),
					Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::UNLOCK_SHARED)
					)))
				)
				.WillOnce(Return(id));

			EXPECT_CALL(*mock, _unlockShared);
		}

		void TearDown() override {
			for(auto mock : mocks) {
				delete mock;
			}
		}
};

/*
 * mutex_server_handle_incoming_requests_test_unlocked_read
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_read) {
	int data = 12;
	expectUnlockedRead(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_read
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_read) {
	int data = 12;
	expectLockedRead(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_shared_read
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_shared_read) {
	int data = 12;
	expectLockedSharedRead(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlocked_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_acquire) {
	int data = 12;
	expectUnlockedAcquire(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_acquire) {
	int data = 12;
	expectLockedAcquire(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_shared_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_shared_acquire) {
	int data = 12;
	expectLockedSharedAcquire(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_release_no_waiting_queue
 */
TEST_F(MutexServerHandleIncomingRequestsTest, release_no_waiting_queue) {
	std::queue<MutexRequest> void_requests;

	int mutex_data = 12;
	int updatedData = 15;
	expectRelease(5, DistributedId(2, 6), updatedData, void_requests, mock_mutex(DistributedId(2, 6), mutex_data));

	server.handleIncomingRequests();
	ASSERT_EQ(mutex_data, 15);
}

/*
 * mutex_server_handle_incoming_requests_test_release_with_waiting_queue
 */
TEST_F(MutexServerHandleIncomingRequestsTest, release_with_waiting_queue) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::READ));
	requests.push(MutexRequest(mock_id, 2, MutexRequestType::LOCK_SHARED));
	requests.push(MutexRequest(mock_id, 5, MutexRequestType::READ));


	int mutex_data = 12;
	int updatedData = 15;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectRelease(5, mock_id, updatedData, requests, mock);
	{
		InSequence s;

		expectReadResponse(3, mock_id, mock);
		expectLockSharedResponse(2, mock_id, mock);
		expectReadResponse(5, mock_id, mock);
	}
	
	server.handleIncomingRequests();
	ASSERT_EQ(mutex_data, 15);
}

/*
 * mutex_server_handle_incoming_requests_test_release_with_pending_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, release_with_pending_lock) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::LOCK));

	int mutex_data = 12;
	int updatedData = 15;

	auto* mock = mock_mutex(mock_id, mutex_data);
	expectRelease(5, mock_id, updatedData, requests, mock);

	expectLockResponse(3, mock_id, mock);
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_release_with_pending_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, release_with_pending_acquire) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::ACQUIRE));

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlock(5, mock_id, requests, mock);

	expectAcquireResponse(3, mock_id, 12, mock);
	
	server.handleIncomingRequests();
}
/*
 * mutex_server_handle_incoming_requests_test_unlocked_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_lock) {
	int data = 0; // unused
	expectUnlockedLock(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_lock) {
	int data = 0; // unused
	expectLockedLock(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_shared_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_shared_lock) {
	int data = 0; // unused
	expectLockedSharedLock(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlocked_lock_shared
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_lock_shared) {
	int data = 0; // unused
	expectUnlockedLockShared(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_lock_shared
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_lock_shared) {
	int data = 0; // unused
	expectLockedLockShared(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_locked_shared_lock_shared
 */
TEST_F(MutexServerHandleIncomingRequestsTest, locked_shared_lock_shared) {
	int data = 0; // unused
	expectLockedSharedLockShared(5, DistributedId(2, 6), mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlock_no_waiting_queues
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlock_no_waiting_queues) {
	std::queue<MutexRequest> void_requests;

	int data = 0; // unused
	expectUnlock(5, DistributedId(2, 6), void_requests, mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlock_with_waiting_queue
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlock_with_waiting_queue) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::READ));
	requests.push(MutexRequest(mock_id, 2, MutexRequestType::LOCK_SHARED));
	requests.push(MutexRequest(mock_id, 5, MutexRequestType::READ));


	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlock(5, mock_id, requests, mock);
	{
		InSequence s;

		expectReadResponse(3, mock_id, mock);
		expectLockSharedResponse(2, mock_id, mock);
		expectReadResponse(5, mock_id, mock);
	}
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlock_with_pending_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlock_with_pending_lock) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::LOCK));

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlock(5, mock_id, requests, mock);

	expectLockResponse(3, mock_id, mock);
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlock_with_pending_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlock_with_pending_acquire) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::ACQUIRE));

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlock(5, mock_id, requests, mock);

	expectAcquireResponse(3, mock_id, 12, mock);
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_partial_unlock_shared
 */
TEST_F(MutexServerHandleIncomingRequestsTest, partial_unlock_shared) {
	std::queue<MutexRequest> void_requests;

	int data = 0; // unused
	expectUnlockShared(5, DistributedId(2, 6), 4, void_requests, mock_mutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_last_unlock_shared_no_waiting_queue
 */
TEST_F(MutexServerHandleIncomingRequestsTest, last_unlock_shared_no_waiting_queue) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlockShared(5, mock_id, 0, requests, mock);
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_last_unlock_shared_with_waiting_queue
 */
TEST_F(MutexServerHandleIncomingRequestsTest, last_unlock_shared_with_waiting_queue) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::READ));
	requests.push(MutexRequest(mock_id, 2, MutexRequestType::LOCK_SHARED));
	requests.push(MutexRequest(mock_id, 5, MutexRequestType::READ));


	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlockShared(5, mock_id, 0, requests, mock);
	{
		InSequence s;

		expectReadResponse(3, mock_id, mock);
		expectLockSharedResponse(2, mock_id, mock);
		expectReadResponse(5, mock_id, mock);
	}
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_last_unlock_shared_with_pending_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, last_unlock_shared_with_pending_lock) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::LOCK));

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlockShared(5, mock_id, 0, requests, mock);

	expectLockResponse(3, mock_id, mock);
	
	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_last_unlock_shared_with_pending_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, last_unlock_shared_with_pending_acquire) {
	DistributedId mock_id {2, 6};
	std::queue<MutexRequest> requests;
	requests.push(MutexRequest(mock_id, 3, MutexRequestType::ACQUIRE));

	int mutex_data = 12;
	auto* mock = mock_mutex(mock_id, mutex_data);
	expectUnlockShared(5, mock_id, 0, requests, mock);

	expectAcquireResponse(3, mock_id, 12, mock);
	
	server.handleIncomingRequests();
}
/*
 * mutex_server_handle_incoming_requests_all
 */
TEST_F(MutexServerHandleIncomingRequestsTest, all) {
	std::array<int, 7> data {0, 0, 0, 0, 0, 0, 0};

	expectUnlockedRead(0, DistributedId(0, 0), mock_mutex(DistributedId(0, 0), data[0]));
	expectUnlockedAcquire(1, DistributedId(0, 1), mock_mutex(DistributedId(0, 1), data[1]));
	expectUnlockedLock(2, DistributedId(0, 2), mock_mutex(DistributedId(0, 2), data[2]));

	std::queue<MutexRequest> void_requests;
	expectRelease(3, DistributedId(0, 3), 10, void_requests, mock_mutex(DistributedId(0, 3), data[3]));

	expectUnlock(4, DistributedId(0, 4), void_requests, mock_mutex(DistributedId(0, 4), data[4]));

	expectUnlockedLockShared(5, DistributedId(0, 5), mock_mutex(DistributedId(0, 5), data[5]));

	expectUnlockShared(6, DistributedId(0, 6), 0, void_requests, mock_mutex(DistributedId(0, 6), data[6]));

	server.handleIncomingRequests();
	ASSERT_EQ(data[3], 10);
}
