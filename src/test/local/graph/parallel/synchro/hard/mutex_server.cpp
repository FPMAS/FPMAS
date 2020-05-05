/* INDEX */
/*
 * # MutexServerTest
 * ## mutex_server_test_manage
 * ## mutex_server_test_remove
 *
 * # MutexServerHandleIncomingRequestsTest
 * ## mutex_server_handle_incoming_requests_test_unlocked_read
 * ## mutex_server_handle_incoming_requests_test_unlocked_acquire
 * ## mutex_server_handle_incoming_requests_test_release_no_waiting_queues
 * ## mutex_server_handle_incoming_requests_test_unlocked_lock
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
using ::testing::Invoke;
using ::testing::Pair;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

using FPMAS::graph::parallel::synchro::hard::Epoch;
using FPMAS::graph::parallel::synchro::hard::Tag;
using FPMAS::graph::parallel::synchro::hard::MutexServer;
using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;

/*******************/
/* MutexServerTest */
/*******************/
class MutexServerTest : public ::testing::Test {
	protected:
	MockMpiCommunicator<3, 8> comm;

	MutexServer<int, MockMpi> server {comm};
};

/*
 * mutex_server_test_manage
 */
TEST_F(MutexServerTest, manage) {
	MockHardSyncMutex<int> mockMutex;
	server.manage(DistributedId(5, 4), &mockMutex);
	server.manage(DistributedId(0, 2), &mockMutex);

	ASSERT_THAT(server.getManagedMutexes(), UnorderedElementsAre(
		Pair(DistributedId(5, 4), _),
		Pair(DistributedId(0, 2), _)
	));
}

TEST_F(MutexServerTest, remove) {
	MockHardSyncMutex<int> mockMutex;
	server.manage(DistributedId(5, 4), &mockMutex);
	server.manage(DistributedId(0, 2), &mockMutex);

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
				void operator()(int source, int tag, MPI_Status* status) {
					status->MPI_SOURCE = this->source;
					status->MPI_TAG = tag;
				}
		};
		class MockRecv {
			private:
				DistributedId id;
			public:
				MockRecv(DistributedId id)
					: id(id) {}
				void operator()(MPI_Status*, DistributedId& id) {
					id = this->id;
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

		MockHardSyncMutex<int>* mockMutex(DistributedId id, int& data) {
			auto* mock = new MockHardSyncMutex<int>();
			mocks.push_back(mock);
			server.manage(id, mock);

			EXPECT_CALL(*mock, data).WillRepeatedly(ReturnRef(data));
			EXPECT_CALL(*mock, locked).WillRepeatedly(Return(false));
			return mock;
		}
	
		void expectRead(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::READ, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(comm, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::READ))),
					A<DistributedId&>()))
				.WillOnce(Invoke(MockRecv(id)));

			EXPECT_CALL(
				const_cast<MockMpi<int>&>(server.getDataMpi()),
				send(mock->data(), source, Epoch::EVEN | Tag::READ_RESPONSE));
		}

		void expectAcquire(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::ACQUIRE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(comm, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::ACQUIRE))),
					A<DistributedId&>()))
				.WillOnce(Invoke(MockRecv(id)));

			EXPECT_CALL(
				const_cast<MockMpi<int>&>(server.getDataMpi()),
				send(mock->data(), source, Epoch::EVEN | Tag::ACQUIRE_RESPONSE));
			EXPECT_CALL(*mock, _lock);
		}

		void expectRelease(
			int source, DistributedId id, int updatedData,
			std::queue<Request>& requests, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(*mock, requestsToProcess)
				.WillOnce(Return(requests));

			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::RELEASE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(const_cast<MockMpi<DataUpdatePack<int>>&>(server.getDataUpdateMpi()),
				recv(Pointee(AllOf(
					Field(&MPI_Status::MPI_SOURCE, source),
					Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::RELEASE))))
				)
				.WillOnce(Return(DataUpdatePack(id, updatedData)));

			EXPECT_CALL(*mock, _unlock);
		}

		void expectLock(int source, DistributedId id, MockHardSyncMutex<int>* mock) {
			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::LOCK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(comm, recv(
					Pointee(AllOf(
						Field(&MPI_Status::MPI_SOURCE, source),
						Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::LOCK))),
					A<DistributedId&>()))
				.WillOnce(Invoke(MockRecv(id)));

			EXPECT_CALL(comm, send(source, Epoch::EVEN | Tag::LOCK_RESPONSE));
			EXPECT_CALL(*mock, _lock);
		}

		void expectUnlock(
			int source, DistributedId id,
			std::queue<Request>& requests, MockHardSyncMutex<int>* mock) {

			EXPECT_CALL(*mock, requestsToProcess)
				.WillOnce(Return(requests));

			EXPECT_CALL(comm, Iprobe(MPI_ANY_SOURCE, Epoch::EVEN | Tag::UNLOCK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(comm,
				recv(Pointee(AllOf(
					Field(&MPI_Status::MPI_SOURCE, source),
					Field(&MPI_Status::MPI_TAG, Epoch::EVEN | Tag::UNLOCK)
					)), A<DistributedId&>())
				)
				.WillOnce(Invoke(MockRecv(id)));

			EXPECT_CALL(*mock, _unlock);
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
	expectRead(5, DistributedId(2, 6), mockMutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlocked_acquire
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_acquire) {
	int data = 12;
	expectAcquire(5, DistributedId(2, 6), mockMutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_release_no_waiting_queues
 */
TEST_F(MutexServerHandleIncomingRequestsTest, release_no_waiting_queues) {
	std::queue<Request> voidRequests;

	int mutexData = 12;
	int updatedData = 15;
	expectRelease(5, DistributedId(2, 6), updatedData, voidRequests, mockMutex(DistributedId(2, 6), mutexData));

	server.handleIncomingRequests();
	ASSERT_EQ(mutexData, 15);
}

/*
 * mutex_server_handle_incoming_requests_test_unlocked_lock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlocked_lock) {
	int data = 0; // unused
	expectLock(5, DistributedId(2, 6), mockMutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_test_unlock
 */
TEST_F(MutexServerHandleIncomingRequestsTest, unlock) {
	std::queue<Request> voidRequests;

	int data = 0; // unused
	expectUnlock(5, DistributedId(2, 6), voidRequests, mockMutex(DistributedId(2, 6), data));

	server.handleIncomingRequests();
}

/*
 * mutex_server_handle_incoming_requests_all
 */
TEST_F(MutexServerHandleIncomingRequestsTest, all) {
	std::array<int, 5> data {0, 0, 0, 0, 0};

	expectRead(0, DistributedId(0, 0), mockMutex(DistributedId(0, 0), data[0]));
	expectAcquire(1, DistributedId(0, 1), mockMutex(DistributedId(0, 1), data[1]));
	expectLock(2, DistributedId(0, 2), mockMutex(DistributedId(0, 2), data[2]));

	std::queue<Request> voidRequests;
	expectRelease(3, DistributedId(0, 3), 10, voidRequests, mockMutex(DistributedId(0, 3), data[3]));

	expectUnlock(4, DistributedId(0, 4), voidRequests, mockMutex(DistributedId(0, 4), data[4]));

	server.handleIncomingRequests();
	ASSERT_EQ(data[3], 10);
}
