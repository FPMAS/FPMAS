#include "graph/parallel/synchro/ghost/basic_ghost_mode.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Matcher;
using ::testing::Pair;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::UnorderedElementsAre;

using FPMAS::graph::parallel::synchro::ghost::GhostSyncLinker;

class GhostSyncLinkerTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int, MockMutex> MockNode;
		typedef MockDistributedArc<int, MockMutex> MockArc;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mockComm;
		MockDistributedGraph<int, MockNode, MockArc> mockedGraph;

		GhostSyncLinker<MockNode, MockArc, MockMpi>
			linker {mockComm, mockedGraph};


		DistributedId arc1_id = DistributedId(3, 12);
		DistributedId arc2_id = DistributedId(0, 7);
		DistributedId arc3_id = DistributedId(1, 0);

		// Mocked arcs that can be used in import / export
		MockArc arc1 {arc1_id, 8};
		MockArc arc2 {arc2_id, 2};
		MockArc arc3 {arc3_id, 5};

		void SetUp() override {
			// Arc1 set up
			arc1.src = new MockNode();
			arc1.tgt = new MockNode();

			// Arc2 set up
			arc2.src = new MockNode();
			arc2.tgt = new MockNode();

			// Arc3 set up
			arc3.src = new MockNode();
			arc3.tgt = new MockNode();
		}
		
		void arcSetUp(MockArc& arc, int srcRank, int tgtRank) {
			LocationState srcLocation = (srcRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;
			LocationState tgtLocation = (tgtRank == current_rank) ?
				LocationState::LOCAL : LocationState::DISTANT;

			EXPECT_CALL(*static_cast<const MockNode*>(arc.src), state).Times(AnyNumber())
				.WillRepeatedly(Return(srcLocation));
			EXPECT_CALL(*static_cast<const MockNode*>(arc.src), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(srcRank));

			EXPECT_CALL(*static_cast<const MockNode*>(arc.tgt), state).Times(AnyNumber())
				.WillRepeatedly(Return(tgtLocation));
			EXPECT_CALL(*static_cast<const MockNode*>(arc.tgt), getLocation).Times(AnyNumber())
				.WillRepeatedly(Return(tgtRank));

			EXPECT_CALL(arc, state).Times(AnyNumber())
				.WillRepeatedly(Return(
					srcLocation == LocationState::LOCAL && tgtLocation == LocationState::LOCAL ?
					LocationState::LOCAL :
					LocationState::DISTANT
					));
		}

};

TEST_F(GhostSyncLinkerTest, export_link) {
	// Arc1 set up
	arcSetUp(arc1, 6, current_rank); // distant src

	// Arc2 set up
	arcSetUp(arc2, current_rank, 0); // distant tgt

	// Arc3 set up
	arcSetUp(arc3, 8, 0); // distant src and tgt

	linker.link(&arc1);
	linker.link(&arc2);
	linker.link(&arc3);

	auto exportArcMatcher = UnorderedElementsAre(
		Pair(6, ElementsAre(arc1)),
		Pair(8, ElementsAre(arc3)),
		Pair(0, UnorderedElementsAre(arc2, arc3))
		);

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(exportArcMatcher));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(IsEmpty()));

	linker.synchronize();

	delete arc1.src;
	delete arc1.tgt;
	delete arc2.src;
	delete arc2.tgt;
	delete arc3.src;
	delete arc3.tgt;
}

TEST_F(GhostSyncLinkerTest, import_link) {
	std::unordered_map<int, std::vector<MockArc>> importMap {
		{2, {arc1, arc3}},
		{4, {arc2}}
	};

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(IsEmpty()))
		.WillOnce(Return(importMap));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(IsEmpty()));

	EXPECT_CALL(mockedGraph, importArc(Property(
			&FPMAS::api::graph::parallel::DistributedArc<int>::getId, arc1.getId())));
	EXPECT_CALL(mockedGraph, importArc(Property(
			&FPMAS::api::graph::parallel::DistributedArc<int>::getId, arc2.getId())));
	EXPECT_CALL(mockedGraph, importArc(Property(
			&FPMAS::api::graph::parallel::DistributedArc<int>::getId, arc3.getId())));

	linker.synchronize();
}

TEST_F(GhostSyncLinkerTest, import_export_link) {
	arcSetUp(arc2, 0, 4);
	auto exportArcMatcher = UnorderedElementsAre(
		Pair(0, ElementsAre(arc2)),
		Pair(4, ElementsAre(arc2))
		);
	linker.link(&arc2);

	std::unordered_map<int, std::vector<MockArc>> importMap {
		{0, {arc1, arc3}}
	};

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(exportArcMatcher))
		.WillOnce(Return(importMap));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(IsEmpty()));

	EXPECT_CALL(mockedGraph, importArc(Property(
			&FPMAS::api::graph::parallel::DistributedArc<int>::getId, arc1.getId())));
	EXPECT_CALL(mockedGraph, importArc(Property(
			&FPMAS::api::graph::parallel::DistributedArc<int>::getId, arc3.getId())));

	linker.synchronize();

	delete arc2.src;
	delete arc2.tgt;
}

TEST_F(GhostSyncLinkerTest, export_unlink) {
	// Arc1 set up
	arcSetUp(arc1, 6, current_rank); // distant src
	// Arc2 set up
	arcSetUp(arc2, current_rank, 0); // distant tgt
	// Arc3 set up
	arcSetUp(arc3, 8, 0); // distant src and tgt

	linker.unlink(&arc1);
	linker.unlink(&arc2);
	linker.unlink(&arc3);

	auto exportIdMatcher = UnorderedElementsAre(
		Pair(6, ElementsAre(arc1_id)),
		Pair(8, ElementsAre(arc3_id)),
		Pair(0, UnorderedElementsAre(arc2_id, arc3_id))
		);

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(IsEmpty()));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(exportIdMatcher));

	linker.synchronize();

	delete arc1.src;
	delete arc1.tgt;
	delete arc2.src;
	delete arc2.tgt;
	delete arc3.src;
	delete arc3.tgt;

}

TEST_F(GhostSyncLinkerTest, import_unlink) {
	// Arc1 set up
	arcSetUp(arc1, 6, current_rank); // distant src
	// Arc2 set up
	arcSetUp(arc2, current_rank, 0); // distant tgt
	// Arc3 set up
	arcSetUp(arc3, 8, 0); // distant src and tgt

	auto arcs = std::unordered_map<
			DistributedId,
			FPMAS::api::graph::parallel::DistributedArc<int>*,
			FPMAS::api::graph::base::IdHash<DistributedId>> {
		{arc1_id, &arc1},
		{arc2_id, &arc2},
		{arc3_id, &arc3}
	};

	EXPECT_CALL(mockedGraph, getArcs)
		.Times(AnyNumber())
		.WillRepeatedly(ReturnRef(arcs));

	EXPECT_CALL(mockedGraph, getArc(arc1_id)).Times(AnyNumber())
		.WillRepeatedly(Return(&arc1));
	EXPECT_CALL(mockedGraph, getArc(arc2_id)).Times(AnyNumber())
		.WillRepeatedly(Return(&arc2));
	EXPECT_CALL(mockedGraph, getArc(arc3_id)).Times(AnyNumber())
		.WillRepeatedly(Return(&arc3));

	std::unordered_map<int, std::vector<DistributedId>> importMap {
		{6, {arc1_id}},
		{0, {arc2_id, arc3_id}}
	};

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(IsEmpty()));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(IsEmpty()))
		.WillOnce(Return(importMap));

	EXPECT_CALL(mockedGraph, clear(&arc1));
	EXPECT_CALL(mockedGraph, clear(&arc2));
	EXPECT_CALL(mockedGraph, clear(&arc3));

	linker.synchronize();

	delete arc1.src;
	delete arc1.tgt;
	delete arc2.src;
	delete arc2.tgt;
	delete arc3.src;
	delete arc3.tgt;
}

TEST_F(GhostSyncLinkerTest, import_export_unlink) {
	// Arc1 set up
	arcSetUp(arc1, 6, current_rank); // distant src
	// Arc2 set up
	arcSetUp(arc2, current_rank, 0); // distant tgt
	// Arc3 set up
	arcSetUp(arc3, 8, 0); // distant src and tgt

	auto arcs = std::unordered_map<
			DistributedId,
			FPMAS::api::graph::parallel::DistributedArc<int>*,
			FPMAS::api::graph::base::IdHash<DistributedId>> {
		{arc2_id, &arc2},
		{arc3_id, &arc3}
	};

	EXPECT_CALL(mockedGraph, getArcs)
		.Times(AnyNumber())
		.WillRepeatedly(ReturnRef(arcs));

	EXPECT_CALL(mockedGraph, getArc(arc2_id)).Times(AnyNumber())
		.WillRepeatedly(Return(&arc2));
	EXPECT_CALL(mockedGraph, getArc(arc3_id)).Times(AnyNumber())
		.WillRepeatedly(Return(&arc3));

	auto exportIdMatcher = ElementsAre(
		Pair(6, ElementsAre(arc1_id))
		);
	linker.unlink(&arc1);

	std::unordered_map<int, std::vector<DistributedId>> importMap {
		{0, {arc2_id, arc3_id}}
	};

	EXPECT_CALL(const_cast<MockMpi<MockArc>&>(linker.getArcMpi()), migrate(IsEmpty()));
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(linker.getDistIdMpi()), migrate(exportIdMatcher))
		.WillOnce(Return(importMap));

	EXPECT_CALL(mockedGraph, clear(&arc2));
	EXPECT_CALL(mockedGraph, clear(&arc3));

	linker.synchronize();

	delete arc1.src;
	delete arc1.tgt;
	delete arc2.src;
	delete arc2.tgt;
	delete arc3.src;
	delete arc3.tgt;
}
