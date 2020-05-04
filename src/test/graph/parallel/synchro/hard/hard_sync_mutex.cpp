#include "gtest/gtest.h"

#include "api/graph/parallel/synchro/hard/mock_hard_sync_mutex.h"
#include "graph/parallel/synchro/hard/hard_sync_mutex.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::NotNull;
using ::testing::WhenDynamicCastTo;

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::graph::parallel::synchro::hard::HardSyncMutex;

class HardSyncMutexTest : public ::testing::Test {
	protected:
		DistributedId id {2, 0};
		int data = 14;
		LocationState state;
		int location;
		MockMutexClient<int> mockMutexClient;
		MockMutexServer<int> mockMutexServer;

		HardSyncMutex<int> hardSyncMutex {id, data, state, location, mockMutexClient, mockMutexServer};
};

TEST_F(HardSyncMutexTest, local_read) {
	state = LocationState::LOCAL;

	ASSERT_EQ(hardSyncMutex.read(), 14);
	ASSERT_THAT(hardSyncMutex.readRequests(), SizeIs(1));
	ASSERT_THAT(hardSyncMutex.acquireRequests(), IsEmpty());
}
