/*********/
/* INDEX */
/*********/
/*
 * # READ
 * ## hard_sync_mutex_unlocked_local_read
 * ## hard_sync_mutex_locked_local_read
 * ## hard_sync_mutex_shared_locked_local_read
 * ## hard_sync_mutex_distant_read
 *
 * ## hard_sync_mutex_partial_local_release_read
 * ## hard_sync_mutex_last_local_release_read
 * ## hard_sync_mutex_distant_release_read
 *
 * # ACQUIRE
 * ## hard_sync_mutex_unlocked_local_acquire
 * ## hard_sync_mutex_locked_local_acquire
 * ## hard_sync_mutex_shared_locked_local_acquire
 * ## hard_sync_mutex_distant_acquire
 *
 * ## hard_sync_mutex_local_release_acquire
 * ## hard_sync_mutex_distant_release_acquire
 *
 * # LOCK
 * ## hard_sync_mutex_unlocked_local_lock
 * ## hard_sync_mutex_locked_local_lock
 * ## hard_sync_mutex_shared_locked_local_lock
 * ## hard_sync_mutex_distant_lock
 *
 * ## hard_sync_mutex_local_unlock
 * ## hard_sync_mutex_distant_unlock
 *
 * # LOCK_SHARED
 * ## hard_sync_mutex_unlocked_local_lock_shared
 * ## hard_sync_mutex_locked_local_lock_shared
 * ## hard_sync_mutex_shared_locked_local_lock_shared
 * ## hard_sync_mutex_distant_lock_shared
 *
 * ## hard_sync_mutex_partial_local_unlock_shared
 * ## hard_sync_mutex_last_local_unlock_shared
 * ## hard_sync_mutex_distant_unlock_shared
 *
 */
#include "gtest/gtest.h"

#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"
#include "graph/parallel/synchro/hard/hard_sync_mutex.h"

using ::testing::AllOf;
using ::testing::Field;
using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::WhenDynamicCastTo;

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::graph::parallel::synchro::hard::HardSyncMutex;
using FPMAS::api::graph::parallel::synchro::hard::MutexRequest;
using FPMAS::api::graph::parallel::synchro::hard::MutexRequestType;

class HardSyncMutexTest : public ::testing::Test {
	protected:
		DistributedId id {2, 0};
		int data = 14;
		LocationState state;
		int location;
		MockMutexClient<int> mock_mutex_client;
		MockMutexServer<int> mock_mutex_server;

		HardSyncMutex<int> hard_sync_mutex {data};

		void SetUp() {
			hard_sync_mutex.setUp(id, state, location, mock_mutex_client, mock_mutex_server);
		}
};

/********/
/* READ */
/********/
/*
 * hard_sync_mutex_unlocked_local_read
 */
TEST_F(HardSyncMutexTest, unlocked_local_read) {
	state = LocationState::LOCAL;

	EXPECT_CALL(mock_mutex_server, wait).Times(0);
	ASSERT_EQ(hard_sync_mutex.read(), 14);
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}


/*
 * hard_sync_mutex_locked_local_read
 */
TEST_F(HardSyncMutexTest, locked_local_read) {
	state = LocationState::LOCAL;

	// Simulates lock from other threads
	mock_mutex_server.lock(&hard_sync_mutex);
	// The current thread should wait : it is assumed that mutex has been
	// released upon return.
	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::READ)
		)));

	ASSERT_EQ(hard_sync_mutex.read(), 14);
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}

/*
 * hard_sync_mutex_shared_locked_local_read
 */
TEST_F(HardSyncMutexTest, shared_locked_local_read) {
	state = LocationState::LOCAL;

	// Simulates shared lock from other threads
	mock_mutex_server.lockShared(&hard_sync_mutex);

	ASSERT_EQ(hard_sync_mutex.read(), 14);
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 2);
}

/*
 * hard_sync_mutex_distant_read
 */
TEST_F(HardSyncMutexTest, distant_read) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, read(id, location))
		.WillOnce(Return(16));

	ASSERT_EQ(hard_sync_mutex.read(), 16);
	ASSERT_EQ(hard_sync_mutex.data(), 16);
}

/*
 * hard_sync_mutex_partial_local_release_read
 */
TEST_F(HardSyncMutexTest, partial_local_release_read) {
	state = LocationState::LOCAL;

	mock_mutex_server.lockShared(&hard_sync_mutex);
	hard_sync_mutex.read();

	EXPECT_CALL(mock_mutex_server, notify).Times(0);

	hard_sync_mutex.releaseRead();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}

/*
 * hard_sync_mutex_last_local_release_read
 */
TEST_F(HardSyncMutexTest, last_local_release_read) {
	state = LocationState::LOCAL;

	hard_sync_mutex.read();

	EXPECT_CALL(mock_mutex_server, notify(id));

	hard_sync_mutex.releaseRead();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 0);
}

/*
 * hard_sync_mutex_distant_release_read
 */
TEST_F(HardSyncMutexTest, distant_release_read) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, releaseRead(id, location));
	hard_sync_mutex.releaseRead();
}

/***********/
/* ACQUIRE */
/***********/
/*
 * hard_sync_mutex_unlocked_local_acquire
 */
TEST_F(HardSyncMutexTest, unlocked_local_acquire) {
	state = LocationState::LOCAL;

	EXPECT_CALL(mock_mutex_server, wait).Times(0);

	ASSERT_EQ(hard_sync_mutex.acquire(), 14);
	ASSERT_TRUE(hard_sync_mutex.locked());
}


/*
 * hard_sync_mutex_locked_local_acquire
 */
TEST_F(HardSyncMutexTest, locked_local_acquire) {
	state = LocationState::LOCAL;

	// Simulates lock from other thread
	mock_mutex_server.lock(&hard_sync_mutex);
	// The current thacquire should wait : it is assumed that mutex has been
	// released upon return.
	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::ACQUIRE)
		)));

	ASSERT_EQ(hard_sync_mutex.acquire(), 14);
	ASSERT_TRUE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_shared_locked_local_acquire
 */
TEST_F(HardSyncMutexTest, shared_locked_local_acquire) {
	state = LocationState::LOCAL;

	// Simulates shared lock from other thacquires
	mock_mutex_server.lockShared(&hard_sync_mutex);

	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::ACQUIRE)
		)));

	ASSERT_EQ(hard_sync_mutex.acquire(), 14);
	ASSERT_TRUE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_distant_acquire
 */
TEST_F(HardSyncMutexTest, distant_acquire) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, acquire(id, location))
		.WillOnce(Return(16));

	ASSERT_EQ(hard_sync_mutex.acquire(), 16);
	ASSERT_EQ(hard_sync_mutex.data(), 16);
}

/*
 * hard_sync_mutex_local_release_acquire
 */
TEST_F(HardSyncMutexTest, local_release_acquire) {
	state = LocationState::LOCAL;

	hard_sync_mutex.acquire();
	ASSERT_TRUE(hard_sync_mutex.locked());

	EXPECT_CALL(mock_mutex_server, notify(id));

	hard_sync_mutex.releaseAcquire();
	ASSERT_FALSE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_distant_release_acquire
 */
TEST_F(HardSyncMutexTest, distant_release_acquire) {
	state = LocationState::DISTANT;

	int& local_data = hard_sync_mutex.data();
	local_data = 21 * 2;

	EXPECT_CALL(mock_mutex_client, releaseAcquire(id, 42, location));

	hard_sync_mutex.releaseAcquire();
}

/********/
/* LOCK */
/********/
/*
 * hard_sync_mutex_unlocked_local_lock
 */
TEST_F(HardSyncMutexTest, unlocked_local_lock) {
	state = LocationState::LOCAL;

	EXPECT_CALL(mock_mutex_server, wait).Times(0);

	hard_sync_mutex.lock();
	ASSERT_TRUE(hard_sync_mutex.locked());
}


/*
 * hard_sync_mutex_locked_local_lock
 */
TEST_F(HardSyncMutexTest, locked_local_lock) {
	state = LocationState::LOCAL;

	// Simulates lock from other thread
	mock_mutex_server.lock(&hard_sync_mutex);
	// The current thlock should wait : it is assumed that mutex has been
	// released upon return.
	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::LOCK)
		)));

	hard_sync_mutex.lock();
	ASSERT_TRUE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_shared_locked_local_lock
 */
TEST_F(HardSyncMutexTest, shared_locked_local_lock) {
	state = LocationState::LOCAL;

	// Simulates shared lock from other thlocks
	mock_mutex_server.lockShared(&hard_sync_mutex);

	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::LOCK)
		)));

	hard_sync_mutex.lock();
	ASSERT_TRUE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_distant_lock
 */
TEST_F(HardSyncMutexTest, distant_lock) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, lock(id, location));

	hard_sync_mutex.lock();
}

/*
 * hard_sync_mutex_local_unlock
 */
TEST_F(HardSyncMutexTest, local_release_lock) {
	state = LocationState::LOCAL;

	hard_sync_mutex.lock();
	ASSERT_TRUE(hard_sync_mutex.locked());

	EXPECT_CALL(mock_mutex_server, notify(id));

	hard_sync_mutex.unlock();
	ASSERT_FALSE(hard_sync_mutex.locked());
}

/*
 * hard_sync_mutex_distant_unlock
 */
TEST_F(HardSyncMutexTest, distant_release_lock) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, unlock(id, location));

	hard_sync_mutex.unlock();
}

/***************/
/* LOCK_SHARED */
/***************/
/*
 * hard_sync_mutex_unlocked_local_lock_shared
 */
TEST_F(HardSyncMutexTest, unlocked_local_lock_shared) {
	state = LocationState::LOCAL;

	EXPECT_CALL(mock_mutex_server, wait).Times(0);

	hard_sync_mutex.lockShared();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}


/*
 * hard_sync_mutex_locked_local_lock_shared
 */
TEST_F(HardSyncMutexTest, locked_local_lock_shared) {
	state = LocationState::LOCAL;

	// Simulates lock from other threads
	mock_mutex_server.lock(&hard_sync_mutex);
	// The current thread should wait : it is assumed that mutex has been
	// released upon return.
	EXPECT_CALL(mock_mutex_server, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, MutexRequest::LOCAL),
		Field(&MutexRequest::type, MutexRequestType::LOCK_SHARED)
		)));

	hard_sync_mutex.lockShared();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}

/*
 * hard_sync_mutex_shared_locked_local_lock_shared
 */
TEST_F(HardSyncMutexTest, shared_locked_local_lock_shared) {
	state = LocationState::LOCAL;

	// Simulates shared lock from other threads
	mock_mutex_server.lockShared(&hard_sync_mutex);

	hard_sync_mutex.lockShared();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 2);
}

/*
 * hard_sync_mutex_distant_lock_shared
 */
TEST_F(HardSyncMutexTest, distant_lock_shared) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, lockShared(id, location));

	hard_sync_mutex.lockShared();
}

/*
 * hard_sync_mutex_partial_local_unlock_shared
 */
TEST_F(HardSyncMutexTest, partial_local_unlock_shared) {
	state = LocationState::LOCAL;

	mock_mutex_server.lockShared(&hard_sync_mutex);
	hard_sync_mutex.lockShared();

	EXPECT_CALL(mock_mutex_server, notify).Times(0);

	hard_sync_mutex.unlockShared();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 1);
}

/*
 * hard_sync_mutex_last_local_unlock_shared
 */
TEST_F(HardSyncMutexTest, last_local_unlock_shared) {
	state = LocationState::LOCAL;

	hard_sync_mutex.lockShared();

	EXPECT_CALL(mock_mutex_server, notify(id));

	hard_sync_mutex.unlockShared();
	ASSERT_EQ(hard_sync_mutex.lockedShared(), 0);
}

/*
 * hard_sync_mutex_distant_unlock_shared
 */
TEST_F(HardSyncMutexTest, distant_unlock_shared) {
	state = LocationState::DISTANT;

	EXPECT_CALL(mock_mutex_client, unlockShared(id, location));
	hard_sync_mutex.unlockShared();
}
