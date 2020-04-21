#include "gtest/gtest.h"

#include "api/communication/mock_communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/mock_distributed_arc.h"
#include "api/graph/parallel/mock_load_balancing.h"
#include "api/graph/parallel/synchro/mock_mutex.h"
#include "api/graph/parallel/synchro/mock_sync_mode.h"

#include "communication/communication.h"
#include "graph/parallel/basic_distributed_graph.h"
#include "graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_node.h"

#include "utils/test.h"

using ::testing::IsEmpty;
using ::testing::_;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::Property;
using ::testing::NiceMock;
using ::testing::ReturnRefOfCopy;

using FPMAS::graph::parallel::BasicDistributedGraph;
using FPMAS::test::ASSERT_CONTAINS;

class BasicDistributedGraphTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			int,
			MockSyncMode<MockMutex, MockSyncLinker>,
			MockDistributedNode,
			MockDistributedArc,
			MockMpiCommunicator<7, 10>,
			MockLoadBalancing> graph;

		typedef decltype(graph) graph_type;
		typedef typename graph_type::node_type node_type; // MockDistributedNode<int, MockMutex>
		typedef typename graph_type::arc_type arc_type;	//MockDistributedArc<int, MockMutex>
	
		void SetUp() override {
			ON_CALL(graph.getMpiCommunicator(), getRank)
				.WillByDefault(Return(7));
			EXPECT_CALL(graph.getMpiCommunicator(), getRank).Times(AnyNumber());
		}

};

/*
 * Local buildNode test
 */
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

/**************/
/* Link tests */
/**************/
class BasicDistributedGraphLinkTest : public BasicDistributedGraphTest {
	protected:
		node_type srcMock;
		MockMutex<int> srcMutex;
		node_type tgtMock;
		MockMutex<int> tgtMutex;
	public:
		void SetUp() override {
			EXPECT_CALL(srcMock, mutex).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(tgtMock, mutex).WillRepeatedly(ReturnRef(tgtMutex));


			EXPECT_CALL(srcMutex, lock);
			EXPECT_CALL(srcMutex, unlock);
			EXPECT_CALL(tgtMutex, lock);
			EXPECT_CALL(tgtMutex, unlock);
			EXPECT_CALL(graph.getSyncLinker(), unlink).Times(0);

			EXPECT_CALL(srcMock, linkOut(_));
			EXPECT_CALL(tgtMock, linkIn(_));
		}

		void checkArcStructure(arc_type* arc, DistributedId expectedArcId) {
			ASSERT_EQ(graph.getArcs().count(expectedArcId), 1);
			ASSERT_EQ(graph.getArc(expectedArcId), arc);
			ASSERT_EQ(arc->getId(), expectedArcId);

			ASSERT_EQ(arc->src, &srcMock);
			ASSERT_EQ(arc->tgt, &tgtMock);

			EXPECT_CALL(*arc, getLayer);
			ASSERT_EQ(arc->getLayer(), 14);

			EXPECT_CALL(*arc, getWeight);
			ASSERT_EQ(arc->getWeight(), 0.5);
		}
};
/*
 * Local link
 */
TEST_F(BasicDistributedGraphLinkTest, local_src_local_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(graph.getSyncLinker(), link).Times(0);

	auto currentId = graph.currentArcId();
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);

	checkArcStructure(arc, currentId);

	ASSERT_EQ(arc->_state, LocationState::LOCAL);
}

/*
 * Local src, distant tgt
 */
TEST_F(BasicDistributedGraphLinkTest, local_src_distant_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::DISTANT));

	const arc_type* linkArcArg;
	auto currentId = graph.currentArcId();
	EXPECT_CALL(graph.getSyncLinker(), link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc, currentId);

	ASSERT_EQ(arc->_state, LocationState::DISTANT);
}

/*
 * Distant src, local tgt
 */
TEST_F(BasicDistributedGraphLinkTest, distant_src_local_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::LOCAL));

	const arc_type* linkArcArg;
	auto currentId = graph.currentArcId();
	EXPECT_CALL(graph.getSyncLinker(), link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc, currentId);

	ASSERT_EQ(arc->_state, LocationState::DISTANT);
}

/*
 * Distant src, distant tgt
 */
// TODO : what should we do with such an arc? Should it be deleted once is has
// been sent?
TEST_F(BasicDistributedGraphLinkTest, distant_src_distant_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::DISTANT));

	const arc_type* linkArcArg;
	auto currentId = graph.currentArcId();
	EXPECT_CALL(graph.getSyncLinker(), link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc, currentId);

	ASSERT_EQ(arc->_state, LocationState::DISTANT);
}

/*********************/
/* Import node tests */
/*********************/
/*
 * Tests suite that test node import process in the following situations :
 * - node import of a new node
 * - node import with a ghost corresponding to this node already in the graph
 */
class BasicDistributedGraphImportTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			int,
			MockSyncMode<MockMutex, MockSyncLinker>,
			MockDistributedNode,
			MockDistributedArc,
			MockMpiCommunicator<>,
			MockLoadBalancing> graph;

		typedef decltype(graph) graph_type;
		typedef typename graph_type::node_type node_type; // MockDistributedNode<int, MockMutex>
		typedef typename graph_type::arc_type arc_type; // MockDistributedArc<int, MockMutex>
		typedef typename decltype(graph)::partition_type partition_type;


		std::unordered_map<int, std::vector<node_type>> importedNodeMocks;
		std::unordered_map<int, std::string> importedNodePack;
		std::unordered_map<int, std::vector<arc_type>> importedArcMocks;
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
			EXPECT_CALL(graph.getSyncMode(), synchronize).Times(1);
			graph.distribute(partition_type());
		}

};

/*
 * Import new node test
 */
TEST_F(BasicDistributedGraphImportTest, import_node) {
	auto mock = node_type(DistributedId(1, 10), 8, 2.1);

	importedNodeMocks[1].push_back(mock);
	distributeTest();
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(1, 10)), 1);
	auto node = graph.getNode(DistributedId(1, 10));
	ASSERT_EQ(node->data(), 8);
	ASSERT_EQ(node->getWeight(), 2.1f);
	ASSERT_EQ(node->state(), LocationState::LOCAL);
}

/*
 * Import node with existing ghost test
 */
TEST_F(BasicDistributedGraphImportTest, import_node_with_existing_ghost) {
	auto node = graph.buildNode(8, 2.1);
	ON_CALL(*node, state()).WillByDefault(Return(LocationState::DISTANT));

	auto mock = node_type(node->getId(), 2, 4.);

	importedNodeMocks[1].push_back(mock);
	EXPECT_CALL(*node, setState(LocationState::LOCAL));

	distributeTest();
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(mock.getId()), 1);
}

/********************/
/* Import arc tests */
/********************/
/*
 * Test suite for the different cases of arc import when :
 * - the two nodes exist and are LOCAL
 * - the two nodes exist, src is DISTANT, tgt is LOCAL
 * - the two nodes exist, src is LOCAL, tgt is DISTANT
 */
class BasicDistributedGraphImportArcTest : public BasicDistributedGraphImportTest {
	protected:
		node_type* src;
		node_type* tgt;
		node_type* mock_src;
		node_type* mock_tgt;

		arc_type* importedArc;

	void SetUp() override {
		src = graph.buildNode();
		tgt = graph.buildNode();

		mock_src = new node_type(src->getId());
		mock_tgt = new node_type(tgt->getId());
		auto mock = arc_type(
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

/*
 * Import arc when the two nodes are LOCAL
 */
TEST_F(BasicDistributedGraphImportArcTest, import_local_arc) {
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}


/*
 * Import with DISTANT src
 */
TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_src) {
	EXPECT_CALL(*src, state).WillRepeatedly(Return(LocationState::DISTANT));
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::DISTANT);
}

/*
 * Import with DISTANT tgt
 */
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
	auto mock = arc_type(DistributedId(3, 12), 2, 2.6);

	mock.src = mock_src;
	mock.tgt = mock_tgt;

	// The same arc is imported from proc 2
	importedArcMocks[2].push_back(mock);
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}

/*************************************/
/* Import arc with missing node test */
/*************************************/
/*
 * In the previous test suite, at least one of the two nodes might be DISTANT,
 * but they already exist in the Graph.
 * However, Arcs might be imported with a source or target node that has not
 * been added to the Graph yet (has LOCAL or DISTANT node), and in this case
 * the missing Node must be created as a new DISTANT node.
 */
class BasicDistributedGraphImportArcWithGhostTest : public BasicDistributedGraphImportTest {
	protected:
		node_type* localNode;
		node_type* mock_one;
		node_type* mock_two;

		arc_type* importedArc;

		void SetUp() override {
			localNode = graph.buildNode();

			mock_one = new node_type(localNode->getId());
			mock_two = new node_type(DistributedId(8, 16));

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

/*
 * LOCAL src, missing tgt
 */
TEST_F(BasicDistributedGraphImportArcWithGhostTest, import_with_missing_tgt) {
	auto mock = arc_type(DistributedId(3, 12),
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

/*
 * Missing src, LOCAL tgt
 */
TEST_F(BasicDistributedGraphImportArcWithGhostTest, import_with_missing_src) {
	auto mock = arc_type(
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

/********************/
/* Distribute tests */
/********************/
class BasicDistributedGraphDistributeTest : public BasicDistributedGraphTest {
	typedef typename graph_type::partition_type partition_type;

	protected:
		std::array<DistributedId, 5> nodeIds;
		partition_type fakePartition;
		void SetUp() override {
			for(int i=0; i < 5;i++) {
				auto node = graph.buildNode();
				nodeIds[i] = node->getId();
			}
			fakePartition[nodeIds[0]] = 7;
			fakePartition[nodeIds[1]] = 1;
			fakePartition[nodeIds[2]] = 1;
			fakePartition[nodeIds[3]] = 0;
			fakePartition[nodeIds[4]] = 0;
			EXPECT_CALL(graph.getSyncMode(), synchronize);
		}
};

/*
 * Balance test.
 * Checks that load balancing is called and migration is performed.
 */
TEST_F(BasicDistributedGraphDistributeTest, balance) {
	// Should call LoadBalancing on all nodes, without fixed nodes
	EXPECT_CALL(
		const_cast<MockLoadBalancing<node_type>&>(graph.getLoadBalancing()),
		balance(graph.getNodes(), IsEmpty()))
		.WillOnce(Return(fakePartition));
	// Migration of nodes + arcs
	EXPECT_CALL(graph.getMpiCommunicator(), allToAll(_)).Times(2);

	// Actual call
	graph.balance();
}

/*
 * Node distribution test (no arcs)
 */
TEST_F(BasicDistributedGraphDistributeTest, distribute_without_link) {
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

	// Mock node import
	std::unordered_map<int, std::string> nodeImport;
	nodeImport[2] = nlohmann::json(
			std::vector<node_type> {
			node_type(DistributedId(2, 5)),
			node_type(DistributedId(4, 3))
			}
			).dump();
	EXPECT_CALL(
			graph.getMpiCommunicator(), allToAll(exportMapMatcher))
	.WillOnce(Return(nodeImport));

	// No arc import
	EXPECT_CALL(graph.getMpiCommunicator(),
			allToAll(IsEmpty())).WillOnce(Return(
				std::unordered_map<int, std::string>()
				));

	graph.distribute(fakePartition);
	ASSERT_EQ(graph.getNodes().size(), 3);
	ASSERT_EQ(graph.getNodes().count(nodeIds[0]), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(2, 5)), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(4, 3)), 1);
}

/****************************/
/* Distribute test with arc */
/****************************/
class BasicDistributedGraphDistributedWithLinkTest : public BasicDistributedGraphDistributeTest {
	protected:
		arc_type* arc1;
		arc_type* arc2;
		void SetUp() override {
			this->BasicDistributedGraphDistributeTest::SetUp();
			// No lock to manage, all links are local
			MockMutex<int> mockMutex;
			EXPECT_CALL(mockMutex, lock).Times(AnyNumber());
			EXPECT_CALL(mockMutex, unlock).Times(AnyNumber());

			EXPECT_CALL(*graph.getNode(nodeIds[0]), mutex)
				.WillRepeatedly(ReturnRef(mockMutex));
			EXPECT_CALL(*graph.getNode(nodeIds[2]), mutex)
				.WillRepeatedly(ReturnRef(mockMutex));
			EXPECT_CALL(*graph.getNode(nodeIds[3]), mutex)
				.WillRepeatedly(ReturnRef(mockMutex));

			arc1 = graph.link(graph.getNode(nodeIds[0]), graph.getNode(nodeIds[2]), 0);

			EXPECT_CALL(*graph.getNode(nodeIds[2]), getIncomingArcs())
				.Times(AnyNumber())
				.WillRepeatedly(Return(std::vector {arc1}));

			arc2 = graph.link(graph.getNode(nodeIds[3]), graph.getNode(nodeIds[0]), 0);
			EXPECT_CALL(*graph.getNode(nodeIds[3]), getOutgoingArcs())
				.Times(AnyNumber())
				.WillRepeatedly(Return(std::vector {arc2}));
		}


};

/*
 * Distribute nodes + arcs
 */
TEST_F(BasicDistributedGraphDistributedWithLinkTest, distribute_with_link_test) {
	auto exportNodeMapMatcher = UnorderedElementsAre(
			Pair(0, AnyOf(
					Eq(nlohmann::json({*graph.getNode(nodeIds[3]), *graph.getNode(nodeIds[4])}).dump()),
					Eq(nlohmann::json({*graph.getNode(nodeIds[4]), *graph.getNode(nodeIds[3])}).dump())
					)),
			Pair(1, AnyOf(
					Eq(nlohmann::json({*graph.getNode(nodeIds[1]), *graph.getNode(nodeIds[2])}).dump()),
					Eq(nlohmann::json({*graph.getNode(nodeIds[2]), *graph.getNode(nodeIds[1])}).dump())
					))
			);
	// No node import
	EXPECT_CALL(
			graph.getMpiCommunicator(), allToAll(exportNodeMapMatcher))
	.WillOnce(Return(std::unordered_map<int, std::string>()));

	auto exportMapMatcher = UnorderedElementsAre(
		Pair(0, Eq(nlohmann::json({*arc2}).dump())),
		Pair(1, Eq(nlohmann::json({*arc1}).dump()))
	);

	std::unordered_map<int, std::string> arcImport;
	arc_type importedArc {DistributedId(4, 6), 2};
	importedArc.src = new node_type(DistributedId(4, 8));
	importedArc.tgt = new node_type(DistributedId(3, 2));
	arcImport[3] = nlohmann::json({importedArc}).dump();

	// Mock arc import
	EXPECT_CALL(graph.getMpiCommunicator(),
			allToAll(exportMapMatcher)).WillOnce(Return(arcImport));

	graph.distribute(fakePartition);

	ASSERT_EQ(graph.getArcs().size(), 3);
	ASSERT_EQ(graph.getArcs().count(DistributedId(4, 6)), 1);
	ASSERT_EQ(graph.getArcs().count(arc1->getId()), 1);
	ASSERT_EQ(graph.getArcs().count(arc2->getId()), 1);

	delete importedArc.src;
	delete importedArc.tgt;
}

/*********************************************/
/* Distribute test (real MPI communications) */
/*********************************************/
/*
 * All the previous tests used locally mocked communications.
 * In those tests, a real MpiCommunicator and DistributedNode and
 * DistributedArcs implementations are used to distribute them accross the
 * available procs.
 */
class Mpi_BasicDistributedGraphBalance : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			int,
			MockSyncMode<MockMutex, MockSyncLinker>,
			FPMAS::graph::parallel::DistributedNode,
			FPMAS::graph::parallel::DistributedArc,
			FPMAS::communication::MpiCommunicator,
			MockLoadBalancing> graph;

		MockLoadBalancing<FPMAS::graph::parallel::DistributedNode<int, MockMutex>>
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
