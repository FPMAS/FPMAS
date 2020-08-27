#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/graph/distributed_node.h"

#include "fpmas/communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_edge.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/graph/mock_location_manager.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::An;
using ::testing::Ref;
using ::testing::NiceMock;

using fpmas::graph::DistributedGraph;
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
		MockDistributedGraph<int, MockDistributedNode<int>, MockDistributedEdge<int>> mock_graph;
		HardDataSync<int> data_sync {comm, server_pack, mock_graph};
};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));
	data_sync.synchronize();
}

class HardSyncLinkerTest : public ::testing::Test {
	protected:
		static const int current_rank = 2;
		MockMpiCommunicator<current_rank, 4> comm;
		NiceMock<MockMutexServer<int>> mutex_server;
		NiceMock<MockLinkServer> server;
		MockLinkClient<int> client;
		MockTerminationAlgorithm termination {comm};

		ServerPack<int> server_pack {comm, termination, mutex_server, server};
		MockDistributedGraph<int, MockDistributedEdge<int>, MockDistributedNode<int>> graph;
		HardDataSync<int> data_sync {comm, server_pack, graph};
		HardSyncLinker<int> sync_linker {graph, client, server_pack, data_sync};

		virtual void SetUp() override {
			ON_CALL(graph, getMpiCommunicator)
				.WillByDefault(ReturnRef(comm));
			EXPECT_CALL(graph, getMpiCommunicator)
				.Times(AnyNumber());
		}
};

class HardSyncLinkerLinkTest : public HardSyncLinkerTest {
	protected:
		MockDistributedEdge<int> edge;
		MockDistributedNode<int> src;
		MockDistributedNode<int> tgt;

		void SetUp() override {
			ON_CALL(graph, getMpiCommunicator)
				.WillByDefault(ReturnRef(comm));
			EXPECT_CALL(graph, getMpiCommunicator)
				.Times(AnyNumber());

			EXPECT_CALL(edge, getSourceNode).Times(AnyNumber())
				.WillRepeatedly(Return(&src));
			EXPECT_CALL(edge, getTargetNode).Times(AnyNumber())
				.WillRepeatedly(Return(&tgt));

			EXPECT_CALL(termination, terminate);
		}

};

TEST_F(HardSyncLinkerLinkTest, link_local_src_local_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(client, link(&edge)).Times(0);
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_local_src_distant_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_distant_src_local_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

// The linked edge must be erased upon synchronization
TEST_F(HardSyncLinkerLinkTest, link_distant_src_distant_tgt) {
	EXPECT_CALL(src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(&edge));
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerTest, unlink_local) {
	MockDistributedEdge<int> edge;
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(client, unlink(&edge)).Times(0);
	sync_linker.unlink(&edge);
}
TEST_F(HardSyncLinkerTest, unlink_distant) {
	MockDistributedEdge<int> edge;
	EXPECT_CALL(edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(client, unlink(&edge));
	sync_linker.unlink(&edge);
}

TEST_F(HardSyncLinkerTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));

	sync_linker.synchronize();
}

class HardSyncLinkerRemoveNodeTest : public HardSyncLinkerTest {
	protected:
		DistributedId node_id {3, 8};
		MockDistributedNode<int>* node_to_remove = new MockDistributedNode<int>(node_id);
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges;
		std::vector<fpmas::api::graph::DistributedEdge<int>*> in_edges;

		DistributedId edge1_id = DistributedId(3, 12);
		DistributedId edge2_id = DistributedId(0, 7);
		DistributedId edge3_id = DistributedId(1, 0);

		// Mocked edges that can be used in import / export
		MockDistributedEdge<int>* edge1 = new MockDistributedEdge<int> {edge1_id, 8};
		MockDistributedEdge<int>* edge2 = new MockDistributedEdge<int> {edge2_id, 2};
		MockDistributedEdge<int>* edge3 = new MockDistributedEdge<int> {edge3_id, 5};


		void SetUp() override {
			edge1->src = new MockDistributedNode<int>;
			edge1->tgt = node_to_remove;
			edge2->src = new MockDistributedNode<int>;
			edge2->tgt = node_to_remove;
			edge3->src = node_to_remove;
			edge3->tgt = new MockDistributedNode<int>;

			out_edges.push_back(edge3);
			in_edges.push_back(edge1);
			in_edges.push_back(edge2);

			ON_CALL(*node_to_remove, getOutgoingEdges())
				.WillByDefault(Return(out_edges));
			EXPECT_CALL(*node_to_remove, getOutgoingEdges())
				.Times(AnyNumber());

			ON_CALL(*node_to_remove, getIncomingEdges())
				.WillByDefault(Return(in_edges));
			EXPECT_CALL(*node_to_remove, getIncomingEdges())
				.Times(AnyNumber());
		}

		void edgeSetUp(MockDistributedEdge<int>& edge, int srcRank, int tgtRank) {
			LocationState srcLocation = (srcRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;
			LocationState tgtLocation = (tgtRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;

			EXPECT_CALL(*static_cast<const MockDistributedNode<int>*>(edge.src), state).Times(AnyNumber())
				.WillRepeatedly(Return(srcLocation));
			EXPECT_CALL(*static_cast<const MockDistributedNode<int>*>(edge.src), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(srcRank));

			EXPECT_CALL(*static_cast<const MockDistributedNode<int>*>(edge.tgt), state).Times(AnyNumber())
				.WillRepeatedly(Return(tgtLocation));
			EXPECT_CALL(*static_cast<const MockDistributedNode<int>*>(edge.tgt), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(tgtRank));

			EXPECT_CALL(edge, state).Times(AnyNumber())
				.WillRepeatedly(Return(
					srcLocation == LocationState::LOCAL && tgtLocation == LocationState::LOCAL ?
					LocationState::LOCAL :
					LocationState::DISTANT
					));
		}

		void TearDown() override {
			delete edge1->src;
			delete edge2->src;
			delete edge3->tgt;
			delete node_to_remove;
			delete edge1;
			delete edge2;
			delete edge3;
		}
};

TEST_F(HardSyncLinkerRemoveNodeTest, remove_local) {
	edgeSetUp(*edge1, current_rank, current_rank);
	edgeSetUp(*edge2, current_rank, current_rank);
	edgeSetUp(*edge3, current_rank, current_rank);

	EXPECT_CALL(client, removeNode).Times(0);
	EXPECT_CALL(graph, unlink(edge1));
	EXPECT_CALL(graph, unlink(edge2));
	EXPECT_CALL(graph, unlink(edge3));

	sync_linker.removeNode(node_to_remove);
}

TEST_F(HardSyncLinkerRemoveNodeTest, remove_distant) {
	edgeSetUp(*edge1, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge2, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge3, 5, current_rank); // node_to_remove = src

	EXPECT_CALL(client, removeNode(node_to_remove));

	sync_linker.removeNode(node_to_remove);
}

