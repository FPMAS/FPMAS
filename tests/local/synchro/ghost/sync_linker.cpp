#include "fpmas/synchro/ghost/ghost_mode.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/synchro/mock_mutex.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Matcher;
using ::testing::Pair;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::UnorderedElementsAre;

using fpmas::synchro::ghost::GhostSyncLinker;

class GhostSyncLinkerTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int> MockNode;
		typedef MockDistributedEdge<int> MockEdge;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mock_comm;
		MockMpi<fpmas::graph::EdgePtrWrapper<int>> edge_mpi {mock_comm};
		MockMpi<DistributedId> id_mpi {mock_comm};
		MockDistributedGraph<int, MockNode, MockEdge> mocked_graph;

		GhostSyncLinker<int>
			linker {edge_mpi, id_mpi, mocked_graph};


		DistributedId edge1_id = DistributedId(3, 12);
		DistributedId edge2_id = DistributedId(0, 7);
		DistributedId edge3_id = DistributedId(1, 0);

		// Mocked edges that can be used in import / export
		MockEdge* edge1 = new MockEdge {edge1_id, 8};
		MockEdge* edge2 = new MockEdge {edge2_id, 2};
		MockEdge* edge3 = new MockEdge {edge3_id, 5};

		void SetUp() override {
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
		
		void edgeSetUp(MockEdge& edge, int srcRank, int tgtRank) {
			LocationState srcLocation = (srcRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;
			LocationState tgtLocation = (tgtRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;

			EXPECT_CALL(*static_cast<const MockNode*>(edge.src), state).Times(AnyNumber())
				.WillRepeatedly(Return(srcLocation));
			EXPECT_CALL(*static_cast<const MockNode*>(edge.src), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(srcRank));

			EXPECT_CALL(*static_cast<const MockNode*>(edge.tgt), state).Times(AnyNumber())
				.WillRepeatedly(Return(tgtLocation));
			EXPECT_CALL(*static_cast<const MockNode*>(edge.tgt), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(tgtRank));

			EXPECT_CALL(edge, state).Times(AnyNumber())
				.WillRepeatedly(Return(
					srcLocation == LocationState::LOCAL && tgtLocation == LocationState::LOCAL ?
					LocationState::LOCAL :
					LocationState::DISTANT
					));
		}

};

TEST_F(GhostSyncLinkerTest, export_link) {
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
	EXPECT_CALL(id_mpi, migrate(IsEmpty()));

	// Distant src + distant tgt => erase
	EXPECT_CALL(mocked_graph, erase(edge3));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerTest, import_link) {
	std::unordered_map<int, std::vector<fpmas::graph::EdgePtrWrapper<int>>>
		import_map {
			{2, {edge1, edge3}},
			{4, {edge2}}
		};

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()))
		.WillOnce(Return(import_map));
	EXPECT_CALL(id_mpi, migrate(IsEmpty()));

	EXPECT_CALL(mocked_graph, importEdge(edge1));
	EXPECT_CALL(mocked_graph, importEdge(edge2));
	EXPECT_CALL(mocked_graph, importEdge(edge3));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerTest, import_export_link) {
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
	EXPECT_CALL(id_mpi, migrate(IsEmpty()));

	EXPECT_CALL(mocked_graph, importEdge(edge1));
	EXPECT_CALL(mocked_graph, importEdge(edge3));

	// Distant src + distant tgt => erase
	EXPECT_CALL(mocked_graph, erase(edge2));
	linker.synchronize();
}

TEST_F(GhostSyncLinkerTest, export_unlink) {
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

TEST_F(GhostSyncLinkerTest, import_unlink) {
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

	EXPECT_CALL(mocked_graph, getEdges)
		.Times(AnyNumber())
		.WillRepeatedly(ReturnRef(edges));

	EXPECT_CALL(mocked_graph, getEdge(edge1_id)).Times(AnyNumber())
		.WillRepeatedly(Return(edge1));
	EXPECT_CALL(mocked_graph, getEdge(edge2_id)).Times(AnyNumber())
		.WillRepeatedly(Return(edge2));
	EXPECT_CALL(mocked_graph, getEdge(edge3_id)).Times(AnyNumber())
		.WillRepeatedly(Return(edge3));

	std::unordered_map<int, std::vector<DistributedId>> importMap {
		{6, {edge1_id}},
		{0, {edge2_id, edge3_id}}
	};

	EXPECT_CALL(edge_mpi, migrate(IsEmpty()));
	EXPECT_CALL(id_mpi, migrate(IsEmpty()))
		.WillOnce(Return(importMap));

	EXPECT_CALL(mocked_graph, erase(edge1));
	EXPECT_CALL(mocked_graph, erase(edge2));
	EXPECT_CALL(mocked_graph, erase(edge3));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerTest, import_export_unlink) {
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

	EXPECT_CALL(mocked_graph, getEdges)
		.Times(AnyNumber())
		.WillRepeatedly(ReturnRef(edges));

	EXPECT_CALL(mocked_graph, getEdge(edge2_id)).Times(AnyNumber())
		.WillRepeatedly(Return(edge2));
	EXPECT_CALL(mocked_graph, getEdge(edge3_id)).Times(AnyNumber())
		.WillRepeatedly(Return(edge3));

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

	EXPECT_CALL(mocked_graph, erase(edge2));
	EXPECT_CALL(mocked_graph, erase(edge3));

	linker.synchronize();
}
