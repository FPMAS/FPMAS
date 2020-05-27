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
 */
#include "gtest/gtest.h"

#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"
#include "graph/parallel/synchro/hard/hard_sync_mutex.h"

using ::testing::AllOf;
using ::testing::Field;
using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::NotNull;
using ::testing::WhenDynamicCastTo;

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::graph::parallel::synchro::hard::HardSyncMutex;
using FPMAS::api::graph::parallel::synchro::hard::MutexRequest;

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
		Field(&MutexRequest::source, -1)
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

	EXPECT_CALL(mock_mutex_client, read(id, location));

	hard_sync_mutex.read();
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
