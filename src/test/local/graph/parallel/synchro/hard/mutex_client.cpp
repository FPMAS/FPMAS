/*********/
/* Index */
/*********/
/*
 * # MutexClientTest
 * ## mutex_client_test_read
 * ## mutex_client_test_acquire
 * ## mutex_client_test_release
 * ## mutex_client_test_lock
 *
 * ## MutexClientDeadlockTest
 * ### mutex_client_deadlock_test_read
 * ### mutex_client_deadlock_test_acquire
 * ### mutex_client_deadlock_test_release
 * ### mutex_client_deadlock_test_lock
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
		MockMutexServer<int> mockMutexServer;
		MutexClient<int, MockMpi> mutexClient {comm, mockMutexServer};

		MockHardSyncMutex<int> distantHardSyncMutex;
		MockHardSyncMutex<int> localHardSyncMutex;

		void SetUp() override {
			ON_CALL(mockMutexServer, getEpoch)
				.WillByDefault(Return(Epoch::EVEN));
			EXPECT_CALL(mockMutexServer, getEpoch).Times(AnyNumber());

		}
};

/*
 * mutex_client_test_read
 */
TEST_F(MutexClientTest, read) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(
			const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
			Issend(DistributedId(7, 3), 5, Epoch::EVEN | Tag::READ, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::READ_RESPONSE, _));
		// 4 : Receive read data
		EXPECT_CALL(const_cast<MockMpi<int>&>(mutexClient.getDataMpi()), recv(_)).WillOnce(Return(27));
	}
	// Actual read call
	int data = mutexClient.read(DistributedId(7, 3), 5);
	ASSERT_EQ(data, 27);
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
		EXPECT_CALL(
			const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
			Issend(DistributedId(7, 3), 5, Epoch::EVEN | Tag::ACQUIRE, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::ACQUIRE_RESPONSE, _));
		// 4 : Receive read data
		EXPECT_CALL(const_cast<MockMpi<int>&>(mutexClient.getDataMpi()), recv(_)).WillOnce(Return(27));
	}
	// Actual read call
	int data = mutexClient.acquire(DistributedId(7, 3), 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_test_release
 */
TEST_F(MutexClientTest, release) {
	int data = 42;
	//EXPECT_CALL(distantHardSyncMutex, data).WillRepeatedly(ReturnRef(data));
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(
			const_cast<MockMpi<DataUpdatePack<int>>&>(mutexClient.getDataUpdateMpi()),
			Issend(AllOf(
				Field(&DataUpdatePack<int>::id, DistributedId(7, 3)),
				Field(&DataUpdatePack<int>::updatedData, data)),
				5, Epoch::EVEN | Tag::RELEASE_ACQUIRE, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	// Actual read call
	mutexClient.releaseAcquire(DistributedId(7, 3), data, 5);
}

/*
 * mutex_client_test_lock
 */
TEST_F(MutexClientTest, lock) {
	{
		InSequence seq;
		// 1 : Lock request
		EXPECT_CALL(
			const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
			Issend(DistributedId(7, 3), 5, Epoch::EVEN | Tag::LOCK, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
		// 3 : Probe for read response
		EXPECT_CALL(comm, probe(5, Epoch::EVEN | Tag::LOCK_RESPONSE, _));
		// 4 : Receive response
		EXPECT_CALL(comm, recv(_));
	}
	mutexClient.lock(DistributedId(7, 3), 5);
}

/*
 * mutex_client_test_unlock
 */
TEST_F(MutexClientTest, unlock) {
	{
		InSequence seq;

		// 1 : sends the request
		EXPECT_CALL(
			const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
			Issend(DistributedId(7, 3), 5, Epoch::EVEN | Tag::UNLOCK, _));
		// 2 : Tests request completion : completes immediately
		EXPECT_CALL(comm, test(_)).WillOnce(Return(true));
	}
	mutexClient.unlock(DistributedId(7, 3), 5);
}

/***************************/
/* mutexClientDeadlockTest */
/***************************/
class MutexClientDeadlockTest : public MutexClientTest {
	protected:
		Sequence seq;

		void setUp(Tag sendTag, Tag recvTag) {
			// 1 : sends the request
				EXPECT_CALL(
					const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
					Issend(DistributedId(7, 3), 5, Epoch::EVEN | sendTag, _))
				.InSequence(seq);

			// 2 : Tests request completion : completes after 20 calls
			for(int i = 0; i < 20; i++) {
				EXPECT_CALL(comm, test(_))
					.InSequence(seq)
					.WillOnce(Return(false));
				EXPECT_CALL(mockMutexServer, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(seq);
			}
			EXPECT_CALL(comm, test(_))
				.InSequence(seq)
				.WillOnce(Return(true));

			// 3 : Probe for response
			EXPECT_CALL(comm, probe(5, Epoch::EVEN | recvTag, _))
				.InSequence(seq);
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
	setUp(Tag::READ, Tag::READ_RESPONSE);

	// Receive read data
	EXPECT_CALL(const_cast<MockMpi<int>&>(mutexClient.getDataMpi()), recv(_))
		.InSequence(seq)
		.WillOnce(Return(27));

	// Actual read call
	int data = mutexClient.read(DistributedId(7, 3), 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_deadlock_test_acquire
 *
 * Deadlock can occur in the same circonstances as with read()
 *
 */
TEST_F(MutexClientDeadlockTest, acquire) {
	setUp(Tag::ACQUIRE, Tag::ACQUIRE_RESPONSE);

	// Receive acquired data
	EXPECT_CALL(const_cast<MockMpi<int>&>(mutexClient.getDataMpi()), recv(_))
		.InSequence(seq)
		.WillOnce(Return(27));

	// Actual acquire call
	int data = mutexClient.acquire(DistributedId(7, 3), 5);
	ASSERT_EQ(data, 27);
}

/*
 * mutex_client_deadlock_test_release
 */
TEST_F(MutexClientDeadlockTest, release) {
	int data = 42;

	// Sends the request
	EXPECT_CALL(
		const_cast<MockMpi<DataUpdatePack<int>>&>(mutexClient.getDataUpdateMpi()),
		Issend(AllOf(
			Field(&DataUpdatePack<int>::id, DistributedId(7, 3)),
			Field(&DataUpdatePack<int>::updatedData, data)),
			5, Epoch::EVEN | Tag::RELEASE_ACQUIRE, _))
		.InSequence(seq);


	for(int i = 0; i < 20; i++) {
		EXPECT_CALL(comm, test(_))
			.InSequence(seq)
			.WillOnce(Return(false));
		EXPECT_CALL(mockMutexServer, handleIncomingRequests)
			.Times(AtLeast(1))
			.InSequence(seq);
	}
	// Tests request completion : returns true after 20 tests
	EXPECT_CALL(comm, test(_))
		.InSequence(seq)
		.WillOnce(Return(true));

	// Actual read call
	mutexClient.releaseAcquire(DistributedId(7, 3), data, 5);
}

/*
 * mutex_client_deadlock_test_lock
 */
TEST_F(MutexClientDeadlockTest, lock) {
	setUp(Tag::LOCK, Tag::LOCK_RESPONSE);

	// Receive acquired data
	EXPECT_CALL(comm, recv(_))
		.InSequence(seq);

	// Actual lock call
	mutexClient.lock(DistributedId(7, 3), 5);
}

/*
 * mutex_client_deadlock_test_unlock
 */
TEST_F(MutexClientDeadlockTest, unlock) {

	Sequence seq;
	// Sends the request
	EXPECT_CALL(
			const_cast<MockMpi<DistributedId>&>(mutexClient.getIdMpi()),
			Issend(DistributedId(7, 3), 5, Epoch::EVEN | Tag::UNLOCK, _))
		.InSequence(seq);

	for(int i = 0; i < 20; i++) {
		EXPECT_CALL(comm, test(_))
			.InSequence(seq)
			.WillOnce(Return(false));
		EXPECT_CALL(mockMutexServer, handleIncomingRequests)
			.Times(AtLeast(1))
			.InSequence(seq);
	}
	// Tests request completion returns true after 20 tests
	EXPECT_CALL(comm, test(_))
		.InSequence(seq)
		.WillOnce(Return(true));

	// Actual unlock call
	mutexClient.unlock(DistributedId(7, 3), 5);
}
