#include "synchro/hard/hard_sync_mode.h"
#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_node.h"

#include "communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::Ref;

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::synchro::HardSyncMode;
using FPMAS::synchro::hard::HardDataSync;
using FPMAS::synchro::hard::HardSyncLinker;

class HardDataSyncTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		MockMutexServer<int> mutex_server;
		MockTerminationAlgorithm termination {comm};
		HardDataSync<int> data_sync {comm, mutex_server, termination};

};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(mutex_server)));
	data_sync.synchronize();
}

class HardSyncLinkerTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		MockLinkServer server;
		MockLinkClient<int> client;
		MockTerminationAlgorithm termination {comm};
		HardSyncLinker<int> syncLinker {comm, client, server, termination};

};

TEST_F(HardSyncLinkerTest, link) {
	MockDistributedArc<int> arc;
	EXPECT_CALL(client, link(&arc));
	syncLinker.link(&arc);
}

TEST_F(HardSyncLinkerTest, unlink) {
	MockDistributedArc<int> arc;
	EXPECT_CALL(client, unlink(&arc));
	syncLinker.unlink(&arc);
}

TEST_F(HardSyncLinkerTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server)));

	syncLinker.synchronize();
}
