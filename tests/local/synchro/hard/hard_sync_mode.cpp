#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/parallel/distributed_graph.h"
#include "fpmas/graph/parallel/distributed_edge.h"
#include "fpmas/graph/parallel/distributed_node.h"

#include "fpmas/communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_edge.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::An;
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
		ServerPack<int> server_pack {comm, termination, mutex_server, link_server};
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

		ServerPack<int> server_pack {comm, termination, mutex_server, server};
		MockDistributedGraph<int, MockDistributedEdge<int>, MockDistributedNode<int>> graph;
		HardSyncLinker<int> syncLinker {graph, client, server_pack};
};

class HardSyncLinkerLinkTest : public HardSyncLinkerTest {
	protected:
		MockDistributedEdge<int> edge;
		MockDistributedNode<int> src;
		MockDistributedNode<int> tgt;

		void SetUp() {
			EXPECT_CALL(edge, getSourceNode).Times(AnyNumber())
				.WillRepeatedly(Return(&src));
			EXPECT_CALL(edge, getTargetNode).Times(AnyNumber())
				.WillRepeatedly(Return(&tgt));

			EXPECT_CALL(client, link(&edge));
			EXPECT_CALL(termination, terminate);
		}

};

TEST_F(HardSyncLinkerLinkTest, link_local_src_local_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	syncLinker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::parallel::DistributedEdge<int>*>()))
		.Times(0);
	syncLinker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_local_src_distant_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	syncLinker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::parallel::DistributedEdge<int>*>()))
		.Times(0);
	syncLinker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_distant_src_local_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	syncLinker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::parallel::DistributedEdge<int>*>()))
		.Times(0);
	syncLinker.synchronize();
}

// The linked edge must be erased upon synchronization
TEST_F(HardSyncLinkerLinkTest, link_distant_src_distant_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	syncLinker.link(&edge);

	EXPECT_CALL(graph, erase(&edge));
	syncLinker.synchronize();
}

TEST_F(HardSyncLinkerTest, unlink) {
	MockDistributedEdge<int> edge;
	EXPECT_CALL(client, unlink(&edge));
	syncLinker.unlink(&edge);
}

TEST_F(HardSyncLinkerTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));

	syncLinker.synchronize();
}
