#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/parallel/distributed_graph.h"
#include "fpmas/graph/parallel/distributed_arc.h"
#include "fpmas/graph/parallel/distributed_node.h"

#include "fpmas/communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::Ref;
using ::testing::NiceMock;

using fpmas::graph::parallel::DistributedGraph;
using fpmas::synchro::HardSyncMode;
using fpmas::synchro::hard::HardDataSync;
using fpmas::synchro::hard::HardSyncLinker;
using fpmas::synchro::hard::ServerPack;

class HardDataSyncTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		NiceMock<MockMutexServer<int>> mutex_server;
		NiceMock<MockLinkServer> link_server;
		
		MockTerminationAlgorithm termination {comm};
		ServerPack<int> server_pack {termination, mutex_server, link_server};
		HardDataSync<int> data_sync {comm, server_pack};
};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));
	data_sync.synchronize();
}

class HardSyncLinkerTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		NiceMock<MockMutexServer<int>> mutex_server;
		NiceMock<MockLinkServer> server;
		MockLinkClient<int> client;
		MockTerminationAlgorithm termination {comm};

		ServerPack<int> server_pack {termination, mutex_server, server};
		HardSyncLinker<int> syncLinker {comm, client, server_pack};
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
	EXPECT_CALL(termination, terminate(Ref(server_pack)));

	syncLinker.synchronize();
}
