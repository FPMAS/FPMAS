#include "fpmas/synchro/ghost/ghost_mode.h"

#include "communication/mock_communication.h"
#include "graph/mock_distributed_node.h"
#include "graph/mock_distributed_graph.h"
#include "synchro/mock_mutex.h"
#include "fpmas/graph/distributed_graph.h"

using namespace testing;

using fpmas::synchro::ghost::GhostSyncLinker;

class GhostSyncLinkerTest : public Test {
	protected:
		typedef MockDistributedNode<int, NiceMock> MockNode;
		typedef MockDistributedEdge<int, NiceMock> MockEdge;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mock_comm;
		MockMpi<fpmas::graph::EdgePtrWrapper<int>> edge_mpi {mock_comm};
		MockMpi<DistributedId> id_mpi {mock_comm};
		MockDistributedGraph<int, MockNode, MockEdge, NiceMock> mock_graph;

		GhostSyncLinker<int>
			linker {edge_mpi, id_mpi, mock_graph};

		DistributedId edge1_id = DistributedId(3, 12);
		DistributedId edge2_id = DistributedId(0, 7);
		DistributedId edge3_id = DistributedId(1, 0);

		// Mocked edges that can be used in import / export
		MockEdge* edge1 = new MockEdge {edge1_id, 8};
		MockEdge* edge2 = new MockEdge {edge2_id, 2};
		MockEdge* edge3 = new MockEdge {edge3_id, 5};

		void SetUp() {
			ON_CALL(mock_graph, getMpiCommunicator())
				.WillByDefault(ReturnRef(mock_comm));
		}
		

		void edgeSetUp(MockEdge& edge, int src_rank, int tgt_rank) {
			LocationState src_location = (src_rank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;
			LocationState tgt_location = (tgt_rank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;

			ON_CALL(*static_cast<const MockNode*>(edge.src), state)
				.WillByDefault(Return(src_location));
			ON_CALL(*static_cast<const MockNode*>(edge.src), location)
				.WillByDefault(Return(src_rank));

			ON_CALL(*static_cast<const MockNode*>(edge.tgt), state)
				.WillByDefault(Return(tgt_location));
			ON_CALL(*static_cast<const MockNode*>(edge.tgt), location)
				.WillByDefault(Return(tgt_rank));

			ON_CALL(edge, state)
				.WillByDefault(Return(
					src_location == LocationState::LOCAL && tgt_location == LocationState::LOCAL ?
					LocationState::LOCAL :
					LocationState::DISTANT
					));
		}

};

class GhostSyncLinkerLinkUnlinkTest : public GhostSyncLinkerTest {

		void SetUp() override {
			GhostSyncLinkerTest::SetUp();
			// Edge1 set up
			edge1->src = new MockNode();
			edge1->tgt = new MockNode();

			// Edge2 set up
			edge2->src = new MockNode();
			edge2->tgt = new MockNode();

			// Edge3 set up
			edge3->src = new MockNode();
			edge3->tgt = new MockNode();
		}

		void TearDown() override {
			delete edge1->src;
			delete edge1->tgt;
			delete edge2->src;
			delete edge2->tgt;
			delete edge3->src;
			delete edge3->tgt;
			delete edge1;
			delete edge2;
			delete edge3;
		}

};

TEST_F(GhostSyncLinkerLinkUnlinkTest, export_link) {
	// Edge1 set up
	edgeSetUp(*edge1, 6, current_rank); // distant src

	// Edge2 set up
	edgeSetUp(*edge2, current_rank, 0); // distant tgt

	// Edge3 set up
	edgeSetUp(*edge3, 8, 0); // distant src and tgt

	linker.link(edge1);
	linker.link(edge2);
	linker.link(edge3);

	auto export_edge_matcher = UnorderedElementsAre(
		Pair(6, ElementsAre(edge1)),
		Pair(8, ElementsAre(edge3)),
		Pair(0, UnorderedElementsAre(edge2, edge3))
		);

	EXPECT_CALL(edge_mpi, migrate(export_edge_matcher));
	EXPECT_CALL(id_mpi, migrate(IsEmpty())).Times(2);

	// Distant src + distant tgt => erase
	EXPECT_CALL(mock_graph, erase(edge3));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerLinkUnlinkTest, import_link) {
	std::unordered_map<int, std::vector<fpmas::graph::EdgePtrWrapper<int>>>
		import_map {
			{2, {edge1, edge3}},
			{4, {edge2}}
		};

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()))
		.WillOnce(Return(import_map));
	EXPECT_CALL(id_mpi, migrate(IsEmpty())).Times(2);

	EXPECT_CALL(mock_graph, importEdge(edge1));
	EXPECT_CALL(mock_graph, importEdge(edge2));
	EXPECT_CALL(mock_graph, importEdge(edge3));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerLinkUnlinkTest, import_export_link) {
	edgeSetUp(*edge2, 0, 4);
	auto export_edge_matcher = UnorderedElementsAre(
		Pair(0, ElementsAre(edge2)),
		Pair(4, ElementsAre(edge2))
		);
	linker.link(edge2);

	std::unordered_map<int, std::vector<fpmas::graph::EdgePtrWrapper<int>>>
		import_map {
			{0, {edge1, edge3}}
		};

	EXPECT_CALL(edge_mpi, migrate(export_edge_matcher))
		.WillOnce(Return(import_map));
	EXPECT_CALL(id_mpi, migrate(IsEmpty())).Times(2);

	EXPECT_CALL(mock_graph, importEdge(edge1));
	EXPECT_CALL(mock_graph, importEdge(edge3));

	// Distant src + distant tgt => erase
	EXPECT_CALL(mock_graph, erase(edge2));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerLinkUnlinkTest, export_unlink) {
	// Edge1 set up
	edgeSetUp(*edge1, 6, current_rank); // distant src
	// Edge2 set up
	edgeSetUp(*edge2, current_rank, 0); // distant tgt
	// Edge3 set up
	edgeSetUp(*edge3, 8, 0); // distant src and tgt

	linker.unlink(edge1);
	linker.unlink(edge2);
	linker.unlink(edge3);

	auto export_id_matcher = UnorderedElementsAre(
		Pair(6, ElementsAre(edge1_id)),
		Pair(8, ElementsAre(edge3_id)),
		Pair(0, UnorderedElementsAre(edge2_id, edge3_id))
		);

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));
	EXPECT_CALL(id_mpi, migrate(export_id_matcher));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerLinkUnlinkTest, import_unlink) {
	// Edge1 set up
	edgeSetUp(*edge1, 6, current_rank); // distant src
	// Edge2 set up
	edgeSetUp(*edge2, current_rank, 0); // distant tgt
	// Edge3 set up
	edgeSetUp(*edge3, 8, 0); // distant src and tgt

	auto edges = std::unordered_map<
			DistributedId,
			fpmas::api::graph::DistributedEdge<int>*,
			fpmas::api::graph::IdHash<DistributedId>> {
		{edge1_id, edge1},
		{edge2_id, edge2},
		{edge3_id, edge3}
	};

	ON_CALL(mock_graph, getEdges)
		.WillByDefault(ReturnRef(edges));

	ON_CALL(mock_graph, getEdge(edge1_id))
		.WillByDefault(Return(edge1));
	ON_CALL(mock_graph, getEdge(edge2_id))
		.WillByDefault(Return(edge2));
	ON_CALL(mock_graph, getEdge(edge3_id))
		.WillByDefault(Return(edge3));

	std::unordered_map<int, std::vector<DistributedId>> importMap {
		{6, {edge1_id}},
		{0, {edge2_id, edge3_id}}
	};

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));

	// TODO : Improve this
	EXPECT_CALL(id_mpi, migrate(IsEmpty()))
		// Remove node migration
		.WillOnce(Return(std::unordered_map<int, std::vector<DistributedId>>()))
		// Unlink migration
		.WillOnce(Return(importMap));

	EXPECT_CALL(mock_graph, erase(edge1));
	EXPECT_CALL(mock_graph, erase(edge2));
	EXPECT_CALL(mock_graph, erase(edge3));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerLinkUnlinkTest, import_export_unlink) {
	// Edge1 set up
	edgeSetUp(*edge1, 6, current_rank); // distant src
	// Edge2 set up
	edgeSetUp(*edge2, current_rank, 0); // distant tgt
	// Edge3 set up
	edgeSetUp(*edge3, 8, 0); // distant src and tgt

	auto edges = std::unordered_map<
			DistributedId,
			fpmas::api::graph::DistributedEdge<int>*,
			fpmas::api::graph::IdHash<DistributedId>> {
		{edge2_id, edge2},
		{edge3_id, edge3}
	};

	ON_CALL(mock_graph, getEdges)
		.WillByDefault(ReturnRef(edges));

	ON_CALL(mock_graph, getEdge(edge2_id))
		.WillByDefault(Return(edge2));
	ON_CALL(mock_graph, getEdge(edge3_id))
		.WillByDefault(Return(edge3));

	auto export_id_matcher = ElementsAre(
		Pair(6, ElementsAre(edge1_id))
		);
	linker.unlink(edge1);

	std::unordered_map<int, std::vector<DistributedId>> import_map {
		{0, {edge2_id, edge3_id}}
	};

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));
	EXPECT_CALL(id_mpi, migrate(export_id_matcher))
		.WillOnce(Return(import_map));

	EXPECT_CALL(mock_graph, erase(edge2));
	EXPECT_CALL(mock_graph, erase(edge3));

	linker.synchronize();
}

class GhostSyncLinkerRemoveNodeTest : public GhostSyncLinkerTest {
	protected:
		DistributedId node_id {3, 8};
		MockNode* node_to_remove = new MockNode(node_id);
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out_edges;
		std::vector<fpmas::api::graph::DistributedEdge<int>*> in_edges;

		void SetUp() override {
			GhostSyncLinkerTest::SetUp();

			edge1->src = new MockNode;
			edge1->tgt = node_to_remove;
			edge2->src = new MockNode;
			edge2->tgt = node_to_remove;
			edge3->src = node_to_remove;
			edge3->tgt = new MockNode;

			out_edges.push_back(edge3);
			in_edges.push_back(edge1);
			in_edges.push_back(edge2);

			ON_CALL(*node_to_remove, getOutgoingEdges())
				.WillByDefault(Return(out_edges));

			ON_CALL(*node_to_remove, getIncomingEdges())
				.WillByDefault(Return(in_edges));
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

TEST_F(GhostSyncLinkerRemoveNodeTest, remove_local) {
	edgeSetUp(*edge1, current_rank, current_rank);
	edgeSetUp(*edge2, current_rank, current_rank);
	edgeSetUp(*edge3, current_rank, current_rank);

	EXPECT_CALL(mock_graph, unlink(edge1));
	EXPECT_CALL(mock_graph, unlink(edge2));
	EXPECT_CALL(mock_graph, unlink(edge3));

	linker.removeNode(node_to_remove);

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));
	EXPECT_CALL(id_mpi, migrate(IsEmpty())).Times(2);

	EXPECT_CALL(mock_graph, erase(node_to_remove));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerRemoveNodeTest, remove_local_with_distant_edges) {
	edgeSetUp(*edge1, 2, current_rank); // node_to_remove = tgt
	edgeSetUp(*edge2, current_rank, current_rank); // node_to_remove = tgt
	edgeSetUp(*edge3, current_rank, 5); // node_to_remove = src

	EXPECT_CALL(mock_graph, unlink(edge1))
		.WillOnce(Invoke(&linker, &GhostSyncLinker<int>::unlink));
	EXPECT_CALL(mock_graph, unlink(edge2))
		.WillOnce(Invoke(&linker, &GhostSyncLinker<int>::unlink));
	EXPECT_CALL(mock_graph, unlink(edge3))
		.WillOnce(Invoke(&linker, &GhostSyncLinker<int>::unlink));

	linker.removeNode(node_to_remove);

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));
	// No node removal migration
	EXPECT_CALL(id_mpi, migrate(IsEmpty()));

	auto export_id_matcher = UnorderedElementsAre(
		Pair(2, ElementsAre(edge1_id)),
		Pair(5, ElementsAre(edge3_id))
		);
	EXPECT_CALL(id_mpi, migrate(export_id_matcher));

	EXPECT_CALL(mock_graph, erase(node_to_remove));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerRemoveNodeTest, remove_distant) {
	edgeSetUp(*edge1, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge2, current_rank, 5); // node_to_remove = tgt
	edgeSetUp(*edge3, 5, current_rank); // node_to_remove = src

	linker.removeNode(node_to_remove);

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));

	auto remove_node_matcher = ElementsAre(
			Pair(5, std::vector<DistributedId> {node_id})
			);
	EXPECT_CALL(id_mpi, migrate(remove_node_matcher));

	auto unlink_matcher = UnorderedElementsAre(
			Pair(5, std::vector<DistributedId> {
				edge1_id, edge2_id, edge3_id
				})
			);

	EXPECT_CALL(mock_graph, erase(node_to_remove));
	linker.synchronize();
}
