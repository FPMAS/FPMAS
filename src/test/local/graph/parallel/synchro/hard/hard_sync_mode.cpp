#include "graph/parallel/synchro/hard/hard_sync_mode.h"
#include "graph/parallel/basic_distributed_graph.h"
#include "graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_node.h"

#include "communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"
#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"

using ::testing::Ref;

using FPMAS::graph::parallel::BasicDistributedGraph;
using FPMAS::graph::parallel::synchro::hard::HardSyncMode;
using FPMAS::graph::parallel::synchro::hard::HardDataSync;
using FPMAS::graph::parallel::synchro::hard::HardSyncLinker;

class HardDataSyncTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		MockMutexServer<int> mutexServer;
		HardDataSync<
			MockDistributedNode<int, MockMutex>,
			MockDistributedArc<int, MockMutex>,
			MockTerminationAlgorithm> dataSync {comm, mutexServer};

};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(
		const_cast<MockTerminationAlgorithm&>(dataSync.getTerminationAlgorithm()),
		terminate(Ref(mutexServer))
		);
	dataSync.synchronize();
}

class HardSyncLinkerTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		MockLinkServer server;
		MockLinkClient<MockDistributedArc<int, MockMutex>> client;
		HardSyncLinker<
			MockDistributedArc<int, MockMutex>,
			MockTerminationAlgorithm
		> syncLinker {comm, client, server};

};

TEST_F(HardSyncLinkerTest, link) {
	MockDistributedArc<int, MockMutex> arc;
	EXPECT_CALL(client, link(&arc));
	syncLinker.link(&arc);
}

TEST_F(HardSyncLinkerTest, unlink) {
	MockDistributedArc<int, MockMutex> arc;
	EXPECT_CALL(client, unlink(&arc));
	syncLinker.unlink(&arc);
}

TEST_F(HardSyncLinkerTest, synchronize) {
	EXPECT_CALL(
		const_cast<MockTerminationAlgorithm&>(syncLinker.getTerminationAlgorithm()),
		terminate(Ref(server)));

	syncLinker.synchronize();
}
