#include "gtest/gtest.h"

#include "api/communication/mock_communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/mock_distributed_arc.h"
#include "api/graph/parallel/mock_load_balancing.h"

#include "communication/communication.h"
#include "graph/parallel/basic_distributed_graph.h"
#include "graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_node.h"

using ::testing::IsEmpty;
using ::testing::_;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::Property;
using ::testing::NiceMock;

using FPMAS::graph::parallel::BasicDistributedGraph;

class BasicDistributedGraphTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			MockDistributedNode<int>,
			MockDistributedArc<int>,
			MockMpiCommunicator<7, 10>,
			MockLoadBalancing> graph;

		void SetUp() override {
			ON_CALL(graph.getMpiCommunicator(), getRank)
				.WillByDefault(Return(7));
			EXPECT_CALL(graph.getMpiCommunicator(), getRank).Times(AnyNumber());
		}

};

TEST_F(BasicDistributedGraphTest, buildNode) {

	auto currentId = graph.currentNodeId();
	//ASSERT_EQ(currentId.rank(), 7);
	auto node = FPMAS::api::graph::base::buildNode(graph, 2, 0.5);

	ASSERT_EQ(graph.getNodes().count(currentId), 1);
	ASSERT_EQ(graph.getNode(currentId), node);
	ASSERT_EQ(node->getId(), currentId);

	EXPECT_CALL(*node, state);
	ASSERT_EQ(node->state(), LocationState::LOCAL);

	EXPECT_CALL(*node, data());
	ASSERT_EQ(node->data(), 2);

	EXPECT_CALL(*node, getWeight);
	ASSERT_EQ(node->getWeight(), .5);
}

TEST_F(BasicDistributedGraphTest, local_link) {
	MockDistributedNode<int> srcMock;
	MockDistributedNode<int> tgtMock;

	typedef typename  MockDistributedNode<int>::arc_type arc_type;
	auto currentId = graph.currentArcId();
	EXPECT_CALL(srcMock, linkOut(_));
	EXPECT_CALL(tgtMock, linkIn(_));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);

	ASSERT_EQ(graph.getArcs().count(currentId), 1);
	ASSERT_EQ(graph.getArc(currentId), arc);
	ASSERT_EQ(arc->getId(), currentId);

	ASSERT_EQ(arc->src, &srcMock);
	ASSERT_EQ(arc->tgt, &tgtMock);

	EXPECT_CALL(*arc, getLayer);
	ASSERT_EQ(arc->getLayer(), 14);

	EXPECT_CALL(*arc, getWeight);
	ASSERT_EQ(arc->getWeight(), 0.5);

	ASSERT_EQ(arc->_state, LocationState::LOCAL);
}

class BasicDistributedGraphPartitionTest : public BasicDistributedGraphTest {
	typedef decltype(graph)::partition_type partition_type;
	protected:
		std::array<DistributedId, 5> nodeIds;
		partition_type fakePartition;
		void SetUp() override {
			ON_CALL(graph.getMpiCommunicator(), getRank).WillByDefault(Return(2));
			EXPECT_CALL(graph.getMpiCommunicator(), getRank)
				.Times(AnyNumber());
			for(int i=0; i < 5;i++) {
				auto node = graph.buildNode();
				nodeIds[i] = node->getId();
			}
			fakePartition[nodeIds[0]] = 2;
			fakePartition[nodeIds[1]] = 1;
			fakePartition[nodeIds[2]] = 1;
			fakePartition[nodeIds[3]] = 0;
			fakePartition[nodeIds[4]] = 0;
		}
};

TEST_F(BasicDistributedGraphPartitionTest, balance) {
	EXPECT_CALL(
		const_cast<MockLoadBalancing<MockDistributedNode<int>>&>(graph.getLoadBalancing()),
		balance(graph.getNodes(), IsEmpty()))
		.WillOnce(Return(fakePartition));
	EXPECT_CALL(graph.getMpiCommunicator(), allToAll(_)).Times(2);

	graph.balance();
}

TEST_F(BasicDistributedGraphPartitionTest, distribute) {
	auto exportMapMatcher = UnorderedElementsAre(
		Pair(0, AnyOf(
				Eq(nlohmann::json({*graph.getNode(nodeIds[3]), *graph.getNode(nodeIds[4])}).dump()),
				Eq(nlohmann::json({*graph.getNode(nodeIds[4]), *graph.getNode(nodeIds[3])}).dump())
				)),
		Pair(1, AnyOf(
				Eq(nlohmann::json({*graph.getNode(nodeIds[1]), *graph.getNode(nodeIds[2])}).dump()),
				Eq(nlohmann::json({*graph.getNode(nodeIds[2]), *graph.getNode(nodeIds[1])}).dump())
				))
	);

	EXPECT_CALL(
			graph.getMpiCommunicator(), allToAll(exportMapMatcher))
	.WillOnce(Return(std::unordered_map<int, std::string>()));
	EXPECT_CALL(graph.getMpiCommunicator(),
			allToAll(IsEmpty())).WillOnce(Return(
				std::unordered_map<int, std::string>()
				));

	graph.distribute(fakePartition);
}

class BasicDistributedGraphImportTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int> node_mock;
		typedef MockDistributedArc<int> arc_mock;
		BasicDistributedGraph<
			node_mock,
			arc_mock,
			MockMpiCommunicator<>,
			MockLoadBalancing> graph;
		typedef typename decltype(graph)::partition_type partition_type;


		std::unordered_map<int, std::vector<node_mock>> importedNodeMocks;
		std::unordered_map<int, std::string> importedNodePack;
		std::unordered_map<int, std::vector<arc_mock>> importedArcMocks;
		std::unordered_map<int, std::string> importedArcPack;

		void distributeTest() {
			for(auto item : importedNodeMocks) {
				importedNodePack[item.first] = nlohmann::json(item.second).dump();
			}
			for(auto item : importedArcMocks) {
				importedArcPack[item.first] = nlohmann::json(item.second).dump();
			}
			EXPECT_CALL(
					graph.getMpiCommunicator(), allToAll)
				.WillOnce(Return(importedNodePack))
				.WillOnce(Return(importedArcPack));
			graph.distribute(partition_type());
		}

};

TEST_F(BasicDistributedGraphImportTest, import_node) {
	auto mock = node_mock(DistributedId(1, 10), 8, 2.1);

	importedNodeMocks[1].push_back(mock);
	distributeTest();
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(1, 10)), 1);
	auto node = graph.getNode(DistributedId(1, 10));
	ASSERT_EQ(node->data(), 8);
	ASSERT_EQ(node->getWeight(), 2.1f);
	ASSERT_EQ(node->state(), LocationState::LOCAL);
}

TEST_F(BasicDistributedGraphImportTest, import_node_with_existing_ghost) {
	auto node = graph.buildNode(8, 2.1);
	ON_CALL(*node, state()).WillByDefault(Return(LocationState::DISTANT));

	auto mock = node_mock(node->getId(), 2, 4.);

	importedNodeMocks[1].push_back(mock);
	EXPECT_CALL(*node, setState(LocationState::LOCAL));

	distributeTest();
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(mock.getId()), 1);
}

/*
 * Test suite for the different cases of arc import when :
 * - the two nodes exist and are LOCAL
 * - the two nodes exist, src is DISTANT, tgt is LOCAL
 * - the two nodes exist, src is LOCAL, tgt is DISTANT
 */
class BasicDistributedGraphImportArcTest : public BasicDistributedGraphImportTest {
	protected:
		MockDistributedNode<int>* src;
		MockDistributedNode<int>* tgt;
		MockDistributedNode<int>* mock_src;
		MockDistributedNode<int>* mock_tgt;

		MockDistributedArc<int>* importedArc;

	void SetUp() override {
		src = graph.buildNode();
		tgt = graph.buildNode();

		mock_src = new MockDistributedNode<int>(src->getId());
		mock_tgt = new MockDistributedNode<int>(tgt->getId());
		auto mock = arc_mock(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);
		mock.src = mock_src;
		mock.tgt = mock_tgt;

		// Mock to serialize and import from proc 1 (not yet in the graph)
		importedArcMocks[1].push_back(mock);

		EXPECT_CALL(*src, linkOut(_));
		EXPECT_CALL(*tgt, linkIn(_));
	}

	void TearDown() override {
		delete mock_src;
		delete mock_tgt;
	}

	void checkArcStructure() {
		ASSERT_EQ(graph.getArcs().size(), 1);
		ASSERT_EQ(graph.getArcs().count(DistributedId(3, 12)), 1);
		importedArc = graph.getArc(DistributedId(3, 12));
		ASSERT_EQ(importedArc->getLayer(), 2);
		ASSERT_EQ(importedArc->getWeight(), 2.6f);

		ASSERT_EQ(importedArc->src, src);
		ASSERT_EQ(importedArc->tgt, tgt);
	}

};

// Import arc when the two nodes are LOCAL
TEST_F(BasicDistributedGraphImportArcTest, import_local_arc) {
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}


// Import with DISTANT src
TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_src) {
	EXPECT_CALL(*src, state).WillRepeatedly(Return(LocationState::DISTANT));
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::DISTANT);
}

// Import with DISTANT tgt
TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_tgt) {
	EXPECT_CALL(*tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::DISTANT);
}

/*
 * Special edge case that test the import behavior when the same arc is
 * imported twice.
 *
 * This happens notably in the following case :
 * - nodes 0 and 1 are linked and lives in distinct procs
 * - they are both imported to an other proc
 * - in consecuence, because each original proc does not know that the other
 *   node will also be imported to the destination proc, the arc connecting the
 *   two nodes (represented as a ghost arc on both original process) is
 *   imported twice to the destination proc.
 *
 * Obviously, the expected behavior is that no duplicate is created, i.e. the
 * second import has no effect.
 */
TEST_F(BasicDistributedGraphImportArcTest, double_import_edge_case) {
	auto mock = arc_mock(DistributedId(3, 12), 2, 2.6);

	mock.src = mock_src;
	mock.tgt = mock_tgt;

	// The same arc is imported from proc 2
	importedArcMocks[2].push_back(mock);
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}

class BasicDistributedGraphImportArcWithGhostTest : public BasicDistributedGraphImportTest {
	protected:
		MockDistributedNode<int>* localNode;
		MockDistributedNode<int>* mock_one;
		MockDistributedNode<int>* mock_two;

		MockDistributedArc<int>* importedArc;

		void SetUp() override {
			localNode = graph.buildNode();

			mock_one = new MockDistributedNode<int>(localNode->getId());
			mock_two = new MockDistributedNode<int>(DistributedId(8, 16));

			// This node is not contained is the graph, but will be linked.
			ASSERT_EQ(graph.getNodes().count(mock_two->getId()), 0);
		}

		void TearDown() override {
			delete mock_one;
			delete mock_two;
		}

		void checkArcAndGhostStructure() {
			ASSERT_EQ(graph.getArcs().size(), 1);
			ASSERT_EQ(graph.getArcs().count(DistributedId(3, 12)), 1);
			importedArc = graph.getArc(DistributedId(3, 12));
			ASSERT_EQ(importedArc->getLayer(), 2);
			ASSERT_EQ(importedArc->getWeight(), 2.6f);
			ASSERT_EQ(importedArc->_state, LocationState::DISTANT);

			EXPECT_EQ(graph.getNodes().size(), 2);
			EXPECT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
			auto ghostNode = graph.getNode(DistributedId(8, 16));
			EXPECT_EQ(ghostNode->getId(), DistributedId(8, 16));
			EXPECT_EQ(ghostNode->state(), LocationState::DISTANT);
		}
};

TEST_F(BasicDistributedGraphImportArcWithGhostTest, import_with_missing_tgt) {
	auto mock = arc_mock(DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock.src = mock_one;
	mock.tgt = mock_two; // This node is node yet contained in the graph

	// Mock to serialize and import from proc 1 (not yet in the graph)
	importedArcMocks[1].push_back(mock);

	EXPECT_CALL(*localNode, linkIn(_)).Times(0);
	EXPECT_CALL(*localNode, linkOut(_));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(importedArc->src, localNode);
	ASSERT_EQ(importedArc->tgt, graph.getNode(DistributedId(8, 16)));
}

TEST_F(BasicDistributedGraphImportArcWithGhostTest, import_with_missing_src) {
	auto mock = arc_mock(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock.src = mock_two; // This node is node yet contained in the graph

	mock.tgt = mock_one;

	// Mock to serialize and import from proc 1 (not yet in the graph)
	importedArcMocks[1].push_back(mock);

	EXPECT_CALL(*localNode, linkOut(_)).Times(0);
	EXPECT_CALL(*localNode, linkIn(_));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(importedArc->tgt, localNode);
	ASSERT_EQ(importedArc->src, graph.getNode(DistributedId(8, 16)));
}

class Mpi_BasicDistributedGraphBalance : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			FPMAS::graph::parallel::DistributedNode<int>,
			FPMAS::graph::parallel::DistributedArc<int>,
			FPMAS::communication::MpiCommunicator,
			MockLoadBalancing> graph;

		MockLoadBalancing<FPMAS::graph::parallel::DistributedNode<int>>
			::partition_type partition;

		void SetUp() override {
			if(graph.getMpiCommunicator().getRank() == 0) {
				auto firstNode = graph.buildNode();
				auto prevNode = firstNode;
				partition[prevNode->getId()] = 0;
				for(auto i = 1; i < graph.getMpiCommunicator().getSize(); i++) {
					auto nextNode = graph.buildNode();
					partition[nextNode->getId()] = i;
					auto arc = graph.link(prevNode, nextNode, 0);
					prevNode = nextNode;
				}
				graph.link(prevNode, firstNode, 0);
			}
		}
};

TEST_F(Mpi_BasicDistributedGraphBalance, distributed_test) {
	graph.distribute(partition);

	if(graph.getMpiCommunicator().getSize() == 1) {
		ASSERT_EQ(graph.getNodes().size(), 1);
	}
	else if(graph.getMpiCommunicator().getSize() == 2) {
		ASSERT_EQ(graph.getNodes().size(), 2);
	}
	else {
		ASSERT_EQ(graph.getNodes().size(), 3);
	}
	ASSERT_EQ(graph.getLocalNodes().size(), 1);
	auto localNode = graph.getLocalNodes().begin()->second;
	if(graph.getMpiCommunicator().getSize() > 1) {
		ASSERT_EQ(localNode->getOutgoingArcs().size(), 1);
		ASSERT_EQ(localNode->getOutgoingArcs()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getOutgoingArcs()[0]->getTargetNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingArcs().size(), 1);
		ASSERT_EQ(localNode->getIncomingArcs()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingArcs()[0]->getSourceNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(graph.getArcs().size(), 2);
	}
}
