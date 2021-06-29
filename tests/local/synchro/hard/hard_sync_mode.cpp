#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/graph/distributed_node.h"

#include "fpmas/communication/communication.h"
#include "communication/mock_communication.h"
#include "graph/mock_distributed_edge.h"
#include "graph/mock_distributed_node.h"
#include "graph/mock_distributed_graph.h"
#include "graph/mock_location_manager.h"
#include "graph/mock_load_balancing.h"
#include "synchro/mock_mutex.h"
#include "synchro/hard/mock_client_server.h"

using namespace testing;

using fpmas::graph::DistributedGraph;
using fpmas::synchro::HardSyncMode;
using fpmas::synchro::hard::HardDataSync;
using fpmas::synchro::hard::HardSyncLinker;
using fpmas::synchro::hard::ServerPack;

class HardDataSyncTest : public Test {
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

class HardSyncLinkerTest : public Test {
	protected:
		static const int current_rank = 2;
		MockMpiCommunicator<current_rank, 4> comm;
		NiceMock<MockMutexServer<int>> mutex_server;
		NiceMock<MockLinkServer> server;
		MockLinkClient<int> client;
		MockTerminationAlgorithm termination {comm};

		ServerPack<int> server_pack {comm, termination, mutex_server, server};
		MockDistributedGraph<int, MockDistributedEdge<int, NiceMock>, MockDistributedNode<int, NiceMock>> graph;
		HardSyncLinker<int> sync_linker {graph, client, server_pack};

		virtual void SetUp() override {
			ON_CALL(graph, getMpiCommunicator())
				.WillByDefault(ReturnRef(comm));
			EXPECT_CALL(graph, getMpiCommunicator())
				.Times(AnyNumber());
		}
};

class HardSyncLinkerLinkTest : public HardSyncLinkerTest {
	protected:
		MockDistributedEdge<int, NiceMock> edge;
		MockDistributedNode<int, NiceMock> src;
		MockDistributedNode<int, NiceMock> tgt;

		void SetUp() override {
			ON_CALL(graph, getMpiCommunicator())
				.WillByDefault(ReturnRef(comm));
			EXPECT_CALL(graph, getMpiCommunicator())
				.Times(AnyNumber());

			ON_CALL(edge, getSourceNode)
				.WillByDefault(Return(&src));
			ON_CALL(edge, getTargetNode)
				.WillByDefault(Return(&tgt));

			EXPECT_CALL(termination, terminate);
		}

};

TEST_F(HardSyncLinkerLinkTest, link_local_src_local_tgt) {
	ON_CALL(src, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(tgt, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::LOCAL));

	EXPECT_CALL(client, link(&edge)).Times(0);
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_local_src_distant_tgt) {
	ON_CALL(src, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerLinkTest, link_distant_src_local_tgt) {
	ON_CALL(src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(tgt, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(An<fpmas::api::graph::DistributedEdge<int>*>()))
		.Times(0);
	sync_linker.synchronize();
}

// The linked edge must be erased upon synchronization
TEST_F(HardSyncLinkerLinkTest, link_distant_src_distant_tgt) {
	ON_CALL(src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(client, link(&edge));
	sync_linker.link(&edge);

	EXPECT_CALL(graph, erase(&edge));
	sync_linker.synchronize();
}

TEST_F(HardSyncLinkerTest, unlink_local) {
	MockDistributedEdge<int, NiceMock> edge;
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::LOCAL));
	EXPECT_CALL(client, unlink(&edge)).Times(0);
	{
		InSequence s;
		EXPECT_CALL(server, lockUnlink(edge.getId()));
		EXPECT_CALL(server, unlockUnlink(edge.getId()));
	}

	sync_linker.unlink(&edge);
}
TEST_F(HardSyncLinkerTest, unlink_distant) {
	MockDistributedEdge<int, NiceMock> edge {{0, 0}, 0};
	ON_CALL(edge, state)
		.WillByDefault(Return(LocationState::DISTANT));
	{
		InSequence s;
		EXPECT_CALL(server, lockUnlink(edge.getId()));
		EXPECT_CALL(client, unlink(&edge));
		EXPECT_CALL(server, unlockUnlink(edge.getId()));
	}
	sync_linker.unlink(&edge);
}

TEST_F(HardSyncLinkerTest, synchronize) {
	EXPECT_CALL(termination, terminate(Ref(server_pack)));

	sync_linker.synchronize();
}

class HardSyncLinkerRemoveNodeTest : public HardSyncLinkerTest {
	protected:
		DistributedId node_id {3, 8};
		MockDistributedNode<int, NiceMock>* node_to_remove
			= new MockDistributedNode<int, NiceMock>(node_id);
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges;
		std::vector<fpmas::api::graph::DistributedEdge<int>*> in_edges;

		DistributedId edge1_id = DistributedId(3, 12);
		DistributedId edge2_id = DistributedId(0, 7);
		DistributedId edge3_id = DistributedId(1, 0);

		// Mocked edges that can be used in import / export
		MockDistributedEdge<int, NiceMock>* edge1
			= new MockDistributedEdge<int, NiceMock> {edge1_id, 8};
		MockDistributedEdge<int, NiceMock>* edge2
			= new MockDistributedEdge<int, NiceMock> {edge2_id, 2};
		MockDistributedEdge<int, NiceMock>* edge3
			= new MockDistributedEdge<int, NiceMock> {edge3_id, 5};


		void SetUp() override {
			edge1->src = new MockDistributedNode<int, NiceMock>;
			edge1->tgt = node_to_remove;
			edge2->src = new MockDistributedNode<int, NiceMock>;
			edge2->tgt = node_to_remove;
			edge3->src = node_to_remove;
			edge3->tgt = new MockDistributedNode<int, NiceMock>;

			out_edges.push_back(edge3);
			in_edges.push_back(edge1);
			in_edges.push_back(edge2);

			ON_CALL(*node_to_remove, getOutgoingEdges())
				.WillByDefault(ReturnPointee(&out_edges));

			ON_CALL(*node_to_remove, getIncomingEdges())
				.WillByDefault(ReturnPointee(&in_edges));
		}

		void edgeSetUp(MockDistributedEdge<int, NiceMock>& edge, int src_rank, int tgt_rank) {
			LocationState src_location = (src_rank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;
			LocationState tgt_location = (tgt_rank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;

			ON_CALL((*static_cast<const MockDistributedNode<int, NiceMock>*>(edge.src)), state)
				.WillByDefault(Return(src_location));
			ON_CALL((*static_cast<const MockDistributedNode<int, NiceMock>*>(edge.src)), location)
				.WillByDefault(Return(src_rank));

			ON_CALL((*static_cast<const MockDistributedNode<int, NiceMock>*>(edge.tgt)), state)
				.WillByDefault(Return(tgt_location));
			ON_CALL((*static_cast<const MockDistributedNode<int, NiceMock>*>(edge.tgt)), location)
				.WillByDefault(Return(tgt_rank));

			ON_CALL(edge, state)
				.WillByDefault(Return(
					src_location == LocationState::LOCAL && tgt_location == LocationState::LOCAL ?
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
	EXPECT_CALL(graph, unlink(edge1))
		.WillOnce(Invoke([this] (fpmas::api::graph::DistributedEdge<int>* edge) {
					in_edges.erase(std::remove(in_edges.begin(), in_edges.end(), edge));
					}));

	EXPECT_CALL(graph, unlink(edge2))
		.WillOnce(Invoke([this] (fpmas::api::graph::DistributedEdge<int>* edge) {
					in_edges.erase(std::remove(in_edges.begin(), in_edges.end(), edge));
					}));
	EXPECT_CALL(graph, unlink(edge3))
		.WillOnce(Invoke([this] (fpmas::api::graph::DistributedEdge<int>* edge) {
					out_edges.erase(std::remove(out_edges.begin(), out_edges.end(), edge));
					}));

	sync_linker.removeNode(node_to_remove);
}

TEST_F(HardSyncLinkerRemoveNodeTest, remove_distant) {
	edgeSetUp(*edge1, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge2, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge3, 5, current_rank); // node_to_remove = src

	EXPECT_CALL(client, removeNode(node_to_remove));

	sync_linker.removeNode(node_to_remove);
}

