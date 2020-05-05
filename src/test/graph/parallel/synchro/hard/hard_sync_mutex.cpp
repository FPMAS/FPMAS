#include "gtest/gtest.h"

#include "api/graph/parallel/synchro/hard/mock_hard_sync_mutex.h"
#include "graph/parallel/synchro/hard/hard_sync_mutex.h"

using ::testing::AllOf;
using ::testing::Field;
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

	EXPECT_CALL(mockMutexServer, wait(AllOf(
		Field(&Request::id, id),
		Field(&Request::source, -1)
		)));
	ASSERT_EQ(hardSyncMutex.read(), 14);
}
