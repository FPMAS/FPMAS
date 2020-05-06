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
		MockMutexClient<int> mockMutexClient;
		MockMutexServer<int> mockMutexServer;

		HardSyncMutex<int> hardSyncMutex {data};

		void SetUp() {
			hardSyncMutex.setUp(id, state, location, mockMutexClient, mockMutexServer);
		}
};

TEST_F(HardSyncMutexTest, local_read) {
	state = LocationState::LOCAL;

	EXPECT_CALL(mockMutexServer, wait(AllOf(
		Field(&MutexRequest::id, id),
		Field(&MutexRequest::source, -1)
		)));
	ASSERT_EQ(hardSyncMutex.read(), 14);
}
