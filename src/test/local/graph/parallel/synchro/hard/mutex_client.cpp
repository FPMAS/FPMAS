/*********/
/* Index */
/*********/
/*
 * # MutexClientTest
 * ## mutex_client_test_read
 * ## mutex_client_test_release_read
 * ## mutex_client_test_acquire
 * ## mutex_client_test_release_acquire
 * ## mutex_client_test_lock
 * ## mutex_client_test_unlock
 * ## mutex_client_test_shared_lock
 * ## mutex_client_test_shared_unlock
 *
 * ## MutexClientDeadlockTest
 * ### mutex_client_deadlock_test_read
 * ### mutex_client_deadlock_test_release_read
 * ### mutex_client_deadlock_test_acquire
 * ### mutex_client_deadlock_test_release_acquire
 * ### mutex_client_deadlock_test_lock
 * ### mutex_client_deadlock_test_unlock
 * ### mutex_client_deadlock_test_lock_shared
 * ### mutex_client_deadlock_test_unlock_shared
 */
#include "graph/parallel/synchro/hard/mutex_client.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/synchro/hard/mock_hard_sync_mutex.h"
#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"

using ::testing::A;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Assign;
using ::testing::Expectation;
using ::testing::Sequence;
using ::testing::InSequence;
using ::testing::Field;
using ::testing::Not;
using ::testing::Pointee;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::SetArgReferee;
using ::testing::_;

using FPMAS::api::graph::parallel::synchro::hard::Epoch;
using FPMAS::api::graph::parallel::synchro::hard::Tag;
using FPMAS::graph::parallel::synchro::hard::MutexClient;
using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;

class MutexClientTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<3, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<int> data_mpi {comm};
		MockMpi<DataUpdatePack<int>> data_update_mpi {comm};

		MockMutexServer<int> mock_mutex_server;
		MutexClient<int> mutex_client {comm, id_mpi, data_mpi, data_update_mpi, mock_mutex_server};

		MockHardSyncMutex<int> distantHardSyncMutex;
		MockHardSyncMutex<int> localHardSyncMutex;

		DistributedId id {7, 3};

		void SetUp() override {
			ON_CALL(mock_mutex_server, getEpoch)
				.WillByDefault(Return(Epoch::EVEN));
			EXPECT_CALL(mock_mutex_server, getEpoch).Times(AnyNumber());

		}
};

/*
 * mutex_client_test_read
 */
TEST_F(MutexClientTest, read) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::READ, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::READ_RESPONSE, _));
		// 4 : Receive read data
		EXPECT_CALL(data_mpi, recv(_)).WillOnce(Return(27));
	}
	// Actual read call
	int data = mutex_client.read(id, 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_test_read_release
 */
TEST_F(MutexClientTest, read_release) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::UNLOCK_SHARED, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	// Actual release read call
	mutex_client.releaseRead(id, 5);
}

/*
 * mutex_client_test_acquire
 *
 * From the client side, acquire is actually nearly equivalent to read, except
 * that the sent request must be of type Tag::ACQUIRE.
 * It is the role of the server to provide an exclusive access to the distant
 * resource, responding to the acquire requests when appropriate.
 *
 * The only requirement from the client side is that acquire() blocks until the
 * distant resource has been acquired.
 */
TEST_F(MutexClientTest, acquire) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::ACQUIRE, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::ACQUIRE_RESPONSE, _));
		// 4 : Receive read data
		EXPECT_CALL(data_mpi, recv(_)).WillOnce(Return(27));
	}
	// Actual read call
	int data = mutex_client.acquire(id, 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_test_release
 */
TEST_F(MutexClientTest, release_acquire) {
	int data = 42;
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(data_update_mpi,
			Issend(AllOf(
				Field(&DataUpdatePack<int>::id, id),
				Field(&DataUpdatePack<int>::updated_data, data)),
				5, Epoch::EVEN | Tag::RELEASE_ACQUIRE, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	// Actual read call
	mutex_client.releaseAcquire(id, data, 5);
}

/*
 * mutex_client_test_lock
 */
TEST_F(MutexClientTest, lock) {
	{
		InSequence seq;
		// 1 : Lock request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::LOCK, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::LOCK_RESPONSE, _));
		// 4 : Receive response
		EXPECT_CALL(comm, recv(_));
	}
	mutex_client.lock(id, 5);
}

/*
 * mutex_client_test_unlock
 */
TEST_F(MutexClientTest, unlock) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::UNLOCK, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	mutex_client.unlock(id, 5);
}

/*
 * mutex_client_test_lock_shared
 */
TEST_F(MutexClientTest, lock_shared) {
	{
		InSequence seq;
		// 1 : Lock request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::LOCK_SHARED, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::LOCK_SHARED_RESPONSE, _));
		// 4 : Receive response
		EXPECT_CALL(comm, recv(_));
	}
	mutex_client.lockShared(id, 5);
}

/*
 * mutex_client_test_unlock_shared
 */
TEST_F(MutexClientTest, unlock_shared) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | Tag::UNLOCK_SHARED, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	mutex_client.unlockShared(id, 5);
}

/***************************/
/* mutex_clientDeadlockTest */
/***************************/
class MutexClientDeadlockTest : public MutexClientTest {
	private :
		void wait() {
			for(int i = 0; i < 20; i++) {
				EXPECT_CALL(comm, test(_))
					.InSequence(seq)
					.WillOnce(Return(false));
				EXPECT_CALL(mock_mutex_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(seq);
			}
			EXPECT_CALL(comm, test(_))
				.InSequence(seq)
				.WillOnce(Return(true));
		}

	protected:
		Sequence seq;

		void setUpWait(Tag sendTag, DistributedId id) {
			// 1 : sends the request
			EXPECT_CALL(id_mpi, Issend(id, 5, Epoch::EVEN | sendTag, _))
				.InSequence(seq);

			// 2 : Tests request completion : completes after 20 calls
			wait();
		}

		void setUpWaitWithResponse(Tag sendTag, DistributedId id, Tag recvTag) {
			setUpWait(sendTag, id);
		
			// 3 : Probe for response
			EXPECT_CALL(comm, probe(5, Epoch::EVEN | recvTag, _))
				.InSequence(seq);
		}

		void setUpWait(Tag sendTag, DistributedId id, int data) {
			// 1 : sends the request
			EXPECT_CALL(data_update_mpi,
					Issend(AllOf(
							Field(&DataUpdatePack<int>::id, id),
							Field(&DataUpdatePack<int>::updated_data, data)),
						5, Epoch::EVEN | Tag::RELEASE_ACQUIRE, _))
				.InSequence(seq);

			// 2 : Tests request completion : completes after 20 calls
			wait();
		}
};

/*
 * mutex_client_deadlock_test_read
 *
 * In practice, because request sends are synchronous, a deadlock might occur
 * in the following situation :
 *
 *        Proc 1        |       Proc 2
 *  --------------------|----------------------
 *  ask_data_to_proc_2  |  ask_data_to_proc_1
 *  wait_for_response   |  wait_for_response
 *
 * This can be solved if :
 * - request is performed using a non-blocking send
 * - the Proc is still able to respond to other requests until its own request
 *   is complete
 */
TEST_F(MutexClientDeadlockTest, read) {
	setUpWaitWithResponse(Tag::READ, id, Tag::READ_RESPONSE);

	// Receive read data
	EXPECT_CALL(data_mpi, recv(_))
		.InSequence(seq)
		.WillOnce(Return(27));

	// Actual read call
	int data = mutex_client.read(id, 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_deadlock_test_release_read
 */
TEST_F(MutexClientDeadlockTest, release_read) {
	setUpWait(Tag::UNLOCK_SHARED, id);

	// Actual read call
	mutex_client.releaseRead(id, 5);
}

/*
 * mutex_client_deadlock_test_acquire
 *
 * Deadlock can occur in the same circonstances as with read()
 *
 */
TEST_F(MutexClientDeadlockTest, acquire) {
	setUpWaitWithResponse(Tag::ACQUIRE, id, Tag::ACQUIRE_RESPONSE);

	// Receive acquired data
	EXPECT_CALL(data_mpi, recv(_))
		.InSequence(seq)
		.WillOnce(Return(27));

	// Actual acquire call
	int data = mutex_client.acquire(id, 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_deadlock_test_release_acquire
 */
TEST_F(MutexClientDeadlockTest, release_acquire) {
	int data = 42;

	setUpWait(Tag::RELEASE_ACQUIRE, id, data);

	// Actual read call
	mutex_client.releaseAcquire(id, data, 5);
}

/*
 * mutex_client_deadlock_test_lock
 */
TEST_F(MutexClientDeadlockTest, lock) {
	setUpWaitWithResponse(Tag::LOCK, id, Tag::LOCK_RESPONSE);

	// Receive lock response
	EXPECT_CALL(comm, recv(_))
		.InSequence(seq);

	// Actual lock call
	mutex_client.lock(id, 5);
}

/*
 * mutex_client_deadlock_test_unlock
 */
TEST_F(MutexClientDeadlockTest, unlock) {

	setUpWait(Tag::UNLOCK, id);

	// Actual unlock call
	mutex_client.unlock(id, 5);
}

/*
 * mutex_client_deadlock_test_lock_shared
 */
TEST_F(MutexClientDeadlockTest, lock_shared) {
	setUpWaitWithResponse(Tag::LOCK_SHARED, id, Tag::LOCK_SHARED_RESPONSE);

	// Receive lock shared response
	EXPECT_CALL(comm, recv(_))
		.InSequence(seq);

	// Actual lock call
	mutex_client.lockShared(id, 5);
}

/*
 * mutex_client_deadlock_test_unlock_shared
 */
TEST_F(MutexClientDeadlockTest, unlock_shared) {

	setUpWait(Tag::UNLOCK_SHARED, id);

	// Actual unlock call
	mutex_client.unlockShared(id, 5);
}
