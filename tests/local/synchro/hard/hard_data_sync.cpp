#include "fpmas/synchro/hard/hard_data_sync.h"

#include "communication/mock_communication.h"
#include "graph/mock_distributed_graph.h"
#include "synchro/hard/mock_client_server.h"

using namespace testing;

using fpmas::synchro::hard::HardDataSync;
using fpmas::synchro::hard::ServerPackBase;

class HardDataSyncTest : public Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		NiceMock<MockMutexServer<int>> mutex_server;
		NiceMock<MockLinkServer> link_server;
		
		MockTerminationAlgorithm termination {comm};
		ServerPackBase server_pack {comm, termination, mutex_server, link_server};
		MockDistributedGraph<int, MockDistributedNode<int>, MockDistributedEdge<int>> mock_graph;
		HardDataSync<int> data_sync {comm, server_pack, mock_graph};
};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));
	data_sync.synchronize();
}

TEST_F(HardDataSyncTest, partial_synchronize) {
	// There is no need to make particular assumptions about the specified
	// nodes, since this is not relevant in the case of HardDataSync
	// synchronization. Indeed, data exchanges in this mode occur only when
	// required, when read() or acquire() method are called, but not directly
	// when the termination algorithm is performed.
	// So we can consider that all nodes are synchronized in any case when
	// HardDataSync::synchronize() is called.

	auto node1 = new MockDistributedNode<int, NiceMock>;
	auto node2 = new MockDistributedNode<int, NiceMock>;

	EXPECT_CALL(termination, terminate(Ref(server_pack)));
	data_sync.synchronize({node1, node2});

	delete node1;
	delete node2;
}

