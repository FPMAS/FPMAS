/*********/
/* Index */
/*********/
/*
 * # local_build_node
 * # local_link
 * # unlink_tests
 * # import_node_tests
 * # import_arc_tests
 * # import_arc_with_missing_node_test
 * # distribute_tests
 * # distribute_test_with_arc
 * # distribute_test_real_MPI
 */
#include "graph/parallel/basic_distributed_graph.h"

#include "../mocks/communication/mock_communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"
#include "../mocks/graph/parallel/synchro/mock_sync_mode.h"


using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Expectation;
using ::testing::IsEmpty;
using ::testing::IsEmpty;
using ::testing::Key;
using ::testing::NiceMock;
using ::testing::Pair;
using ::testing::Property;
using ::testing::Ref;
using ::testing::ReturnRefOfCopy;
using ::testing::Sequence;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;
using ::testing::_;

using FPMAS::graph::parallel::BasicDistributedGraph;

/********************/
/* local_build_node */
/********************/
class BasicDistributedGraphTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			int,
			MockSyncMode<>,
			MockDistributedNode,
			MockDistributedArc,
			MockMpiSetUp<7, 10>,
			MockLocationManager,
			MockLoadBalancing> graph;

		typedef decltype(graph) graph_type;
		typedef typename graph_type::node_type node_type; // MockDistributedNode<int, MockMutex>
		typedef typename graph_type::arc_type arc_type;	//MockDistributedArc<int, MockMutex>
		typedef typename graph_type::sync_mode_runtime sync_mode_runtime;

		MockMpiCommunicator<7, 10>& comm =
			static_cast<MockMpiCommunicator<7, 10>&>(graph.getMpiCommunicator());

		MockLocationManager<node_type>& locationManager
			= const_cast<MockLocationManager<node_type>&>(
					graph.getLocationManager()
					);

		MockSyncLinker<MockDistributedArc<int, MockMutex>> mockSyncLinker {comm};
		MockDataSync mockDataSync;

	
		void SetUp() override {
			ON_CALL(graph.getSyncModeRuntime(), getSyncLinker)
				.WillByDefault(ReturnRef(mockSyncLinker));
			EXPECT_CALL(graph.getSyncModeRuntime(), getSyncLinker)
				.Times(AnyNumber());
			ON_CALL(graph.getSyncModeRuntime(), getDataSync)
				.WillByDefault(ReturnRef(mockDataSync));
			EXPECT_CALL(graph.getSyncModeRuntime(), getDataSync)
				.Times(AnyNumber());
		}

};

/*
 * Local buildNode test
 */
TEST_F(BasicDistributedGraphTest, buildNode) {

	node_type* addManagedNodeArg;
	MockLocationManager<node_type>& locationManager
		= const_cast<MockLocationManager<node_type>&>(
			static_cast<const MockLocationManager<node_type>&>(
				graph.getLocationManager()
				));
	EXPECT_CALL(locationManager, addManagedNode(_, 7))
		.WillOnce(SaveArg<0>(&addManagedNodeArg));
	node_type* setLocalArg;
	EXPECT_CALL(locationManager, setLocal(_))
		.WillOnce(SaveArg<0>(&setLocalArg));


	auto currentId = graph.currentNodeId();
	ASSERT_EQ(currentId.rank(), 7);
	EXPECT_CALL(const_cast<sync_mode_runtime&>(graph.getSyncModeRuntime()), setUp(currentId, _));

	auto node = FPMAS::api::graph::base::buildNode(graph, 2, 0.5);
	ASSERT_EQ(addManagedNodeArg, node);
	ASSERT_EQ(setLocalArg, node);

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
/* local_link */
/**************/
class BasicDistributedGraphLinkTest : public BasicDistributedGraphTest {
	protected:
		node_type srcMock;
		MockMutex<int> srcMutex;
		node_type tgtMock;
		MockMutex<int> tgtMutex;
		DistributedId currentId = graph.currentArcId();
		arc_type* linkOutArg;
		arc_type* linkInArg;

	public:
		void SetUp() override {
			BasicDistributedGraphTest::SetUp();

			EXPECT_CALL(srcMock, mutex).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(tgtMock, mutex).WillRepeatedly(ReturnRef(tgtMutex));

			EXPECT_CALL(srcMutex, lock);
			EXPECT_CALL(srcMutex, unlock);
			EXPECT_CALL(tgtMutex, lock);
			EXPECT_CALL(tgtMutex, unlock);
			EXPECT_CALL(mockSyncLinker, unlink).Times(0);

			EXPECT_CALL(srcMock, linkOut)
				.WillOnce(SaveArg<0>(&linkOutArg));
			EXPECT_CALL(tgtMock, linkIn)
				.WillOnce(SaveArg<0>(&linkInArg));
		}

		void checkArcStructure(arc_type* arc) {
			ASSERT_EQ(linkInArg, arc);
			ASSERT_EQ(linkOutArg, arc);

			ASSERT_EQ(graph.getArcs().count(currentId), 1);
			ASSERT_EQ(graph.getArc(currentId), arc);
			ASSERT_EQ(arc->getId(), currentId);

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

	EXPECT_CALL(mockSyncLinker, link).Times(0);

	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);

	checkArcStructure(arc);

	ASSERT_EQ(arc->_state, LocationState::LOCAL);
}

/*
 * Local src, distant tgt
 */
TEST_F(BasicDistributedGraphLinkTest, local_src_distant_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::DISTANT));

	const arc_type* linkArcArg;
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc);

	ASSERT_EQ(arc->_state, LocationState::DISTANT);
}

/*
 * Distant src, local tgt
 */
TEST_F(BasicDistributedGraphLinkTest, distant_src_local_tgt) {
	EXPECT_CALL(srcMock, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(tgtMock, state).WillRepeatedly(Return(LocationState::LOCAL));

	const arc_type* linkArcArg;
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc);

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
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(arc);

	ASSERT_EQ(arc->_state, LocationState::DISTANT);
}

/****************/
/* unlink_tests */
/****************/
class BasicDistributedGraphUnlinkTest : public BasicDistributedGraphTest {
	protected:
		node_type* srcMock = new node_type(DistributedId(0, 0));
		MockMutex<int> srcMutex;
		node_type* tgtMock = new node_type(DistributedId(0, 1));
		MockMutex<int> tgtMutex;
		arc_type* arc;

	public:
		void SetUp() override {
			BasicDistributedGraphTest::SetUp();

			EXPECT_CALL(*srcMock, mutex).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(*tgtMock, mutex).WillRepeatedly(ReturnRef(tgtMutex));

			EXPECT_CALL(srcMutex, lock).Times(2);
			EXPECT_CALL(srcMutex, unlock).Times(2);
			EXPECT_CALL(tgtMutex, lock).Times(2);
			EXPECT_CALL(tgtMutex, unlock).Times(2);
		}

		void link(LocationState srcState, LocationState tgtState) {
			EXPECT_CALL(*srcMock, state).WillRepeatedly(Return(srcState));
			EXPECT_CALL(*tgtMock, state).WillRepeatedly(Return(tgtState));

			EXPECT_CALL(*srcMock, linkOut);
			EXPECT_CALL(*tgtMock, linkIn);
			EXPECT_CALL(mockSyncLinker, link).Times(AnyNumber());

			arc = graph.link(srcMock, tgtMock, 0);

			Expectation unlinkSrc = EXPECT_CALL(*srcMock, unlinkOut(arc));
			Expectation unlinkTgt = EXPECT_CALL(*tgtMock, unlinkIn(arc));

			/*
			 * Might be call to clear() DISTANT node after the arc has been
			 * erased
			 */
			EXPECT_CALL(*srcMock, getIncomingArcs()).Times(AnyNumber()).After(unlinkSrc);
			EXPECT_CALL(*srcMock, getOutgoingArcs()).Times(AnyNumber()).After(unlinkSrc);

			EXPECT_CALL(*tgtMock, getIncomingArcs()).Times(AnyNumber()).After(unlinkTgt);
			EXPECT_CALL(*tgtMock, getOutgoingArcs()).Times(AnyNumber()).After(unlinkTgt);
		}
};

TEST_F(BasicDistributedGraphUnlinkTest, local_src_local_tgt) {
	this->link(LocationState::LOCAL, LocationState::LOCAL);
	ASSERT_EQ(arc->_state, LocationState::LOCAL);

	EXPECT_CALL(mockSyncLinker, unlink).Times(0);

	graph.unlink(arc);

	ASSERT_EQ(graph.getArcs().size(), 0);

	delete srcMock;
	delete tgtMock;
}

TEST_F(BasicDistributedGraphUnlinkTest, local_src_distant_tgt) {
	this->link(LocationState::LOCAL, LocationState::DISTANT);
	ASSERT_EQ(arc->_state, LocationState::DISTANT);

	EXPECT_CALL(mockSyncLinker, unlink(arc));
	EXPECT_CALL(locationManager, remove(tgtMock));

	graph.unlink(arc);

	ASSERT_EQ(graph.getArcs().size(), 0);
	delete srcMock;
}

TEST_F(BasicDistributedGraphUnlinkTest, distant_src_local_tgt) {
	this->link(LocationState::DISTANT, LocationState::LOCAL);
	ASSERT_EQ(arc->_state, LocationState::DISTANT);

	EXPECT_CALL(mockSyncLinker, unlink(arc));
	EXPECT_CALL(locationManager, remove(srcMock));

	graph.unlink(arc);

	ASSERT_EQ(graph.getArcs().size(), 0);
	delete tgtMock;
}

/*********************/
/* import_node_tests */
/*********************/
/*
 * Tests suite that test node import process in the following situations :
 * - node import of a new node
 * - node import with a ghost corresponding to this node already in the graph
 */
class BasicDistributedGraphImportNodeTest : public BasicDistributedGraphTest {
	protected:
		typedef typename graph_type::node_type node_type; // MockDistributedNode<int, MockMutex>
		typedef typename graph_type::arc_type arc_type; // MockDistributedArc<int, MockMutex>
		typedef typename decltype(graph)::partition_type partition_type;

		partition_type partition;
		std::unordered_map<int, std::vector<node_type>> importedNodeMocks;
		std::unordered_map<int, std::vector<arc_type>> importedArcMocks;
		
		void SetUp() override {
			BasicDistributedGraphTest::SetUp();
		}

		void distributeTest() {
			EXPECT_CALL(
					(const_cast<MockMpi<node_type>&>(graph.getNodeMpi())),
					migrate
					).WillOnce(Return(importedNodeMocks));
			EXPECT_CALL(
					(const_cast<MockMpi<arc_type>&>(graph.getArcMpi())),
					migrate
					).WillOnce(Return(importedArcMocks));
			EXPECT_CALL(mockSyncLinker, synchronize).Times(1);
			EXPECT_CALL(mockDataSync, synchronize).Times(1);
			graph.distribute(partition);
		}

};

/*
 * Import new node test
 */
TEST_F(BasicDistributedGraphImportNodeTest, import_node) {
	importedNodeMocks[1].push_back(node_type(DistributedId(1, 10), 8, 2.1));
	auto localNodesMatcher = ElementsAre(
		Key(DistributedId(1, 10))
		);

	node_type* setLocalArg;
	EXPECT_CALL(locationManager,updateLocations(localNodesMatcher));
	EXPECT_CALL(locationManager, setLocal(_))
		.WillOnce(SaveArg<0>(&setLocalArg));
	EXPECT_CALL(
		const_cast<sync_mode_runtime&>(graph.getSyncModeRuntime()),
		setUp(DistributedId(1, 10), _)
		);

	distributeTest();

	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(1, 10)), 1);
	auto node = graph.getNode(DistributedId(1, 10));
	ASSERT_EQ(setLocalArg, node);
	ASSERT_EQ(node->data(), 8);
	ASSERT_EQ(node->getWeight(), 2.1f);
}

/*
 * Import node with existing ghost test
 */
TEST_F(BasicDistributedGraphImportNodeTest, import_node_with_existing_ghost) {
	EXPECT_CALL(locationManager, addManagedNode);
	EXPECT_CALL(locationManager, setLocal);
	EXPECT_CALL(
		const_cast<sync_mode_runtime&>(graph.getSyncModeRuntime()),
		setUp(graph.currentNodeId(), _)
		);
	auto node = graph.buildNode(8, 2.1);
	ON_CALL(*node, state()).WillByDefault(Return(LocationState::DISTANT));

	importedNodeMocks[1].push_back(node_type(node->getId(), 2, 4.));
	auto localNodesMatcher = ElementsAre(
			Key(node->getId())
			);

	EXPECT_CALL(locationManager, updateLocations(localNodesMatcher));

	node_type* setLocalArg;
	EXPECT_CALL(locationManager, setLocal(_))
		.WillOnce(SaveArg<0>(&setLocalArg));

	distributeTest();

	ASSERT_EQ(setLocalArg, node);
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(node->getId()), 1);
}

/********************/
/* import_arc_tests */
/********************/
/*
 * Test suite for the different cases of arc import when :
 * - the two nodes exist and are LOCAL
 * - the two nodes exist, src is DISTANT, tgt is LOCAL
 * - the two nodes exist, src is LOCAL, tgt is DISTANT
 */
class BasicDistributedGraphImportArcTest : public BasicDistributedGraphImportNodeTest {
	protected:
		node_type* src;
		node_type* tgt;
		node_type* mock_src;
		node_type* mock_tgt;

		arc_type* importedArc;

	void SetUp() override {
		BasicDistributedGraphImportNodeTest::SetUp();

		EXPECT_CALL(locationManager, addManagedNode).Times(2);
		EXPECT_CALL(locationManager, setLocal).Times(2);
		EXPECT_CALL(graph.getSyncModeRuntime(), setUp).Times(2);
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
	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}


/*
 * Import with DISTANT src
 */
TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_src) {
	EXPECT_CALL(*src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::DISTANT);
}

/*
 * Import with DISTANT tgt
 */
TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_tgt) {
	EXPECT_CALL(*tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

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

	mock.src = new node_type(src->getId());
	mock.tgt = new node_type(tgt->getId());

	// The same arc is imported from proc 2
	importedArcMocks[2].push_back(mock);
	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(importedArc->_state, LocationState::LOCAL);
}

/*************************************/
/* import_arc_with_missing_node_test */
/*************************************/
/*
 * In the previous test suite, at least one of the two nodes might be DISTANT,
 * but they already exist in the Graph.
 * However, Arcs might be imported with a source or target node that has not
 * been added to the Graph yet (has LOCAL or DISTANT node), and in this case
 * the missing Node must be created as a new DISTANT node.
 */
class BasicDistributedGraphImportArcWithGhostTest : public BasicDistributedGraphImportNodeTest {
	protected:
		node_type* localNode;
		node_type* mock_one;
		node_type* mock_two;

		arc_type* importedArc;

		void SetUp() override {
			BasicDistributedGraphTest::SetUp();

			EXPECT_CALL(graph.getSyncModeRuntime(), setUp);
			EXPECT_CALL(locationManager, addManagedNode);
			EXPECT_CALL(locationManager, setLocal);
			localNode = graph.buildNode();

			mock_one = new node_type(localNode->getId());
			mock_two = new node_type(DistributedId(8, 16));

			// This node is not contained is the graph, but will be linked.
			ASSERT_EQ(graph.getNodes().count(mock_two->getId()), 0);

			// SetUp should be call once for the created tgt or src
			EXPECT_CALL(graph.getSyncModeRuntime(), setUp);
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
	mock.tgt = mock_two; // This node is not yet contained in the graph

	// Mock to serialize and import from proc 1 (not yet in the graph)
	importedArcMocks[1].push_back(mock);

	EXPECT_CALL(*localNode, linkIn(_)).Times(0);
	EXPECT_CALL(*localNode, linkOut(_));

	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

	node_type* setDistantArg;
	EXPECT_CALL(locationManager, setDistant(_))
		.WillOnce(SaveArg<0>(&setDistantArg));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distantNode = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(setDistantArg, distantNode);

	ASSERT_EQ(importedArc->src, localNode);
	ASSERT_EQ(importedArc->tgt, distantNode);
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

	mock.src = mock_two; // This node is not yet contained in the graph
	mock.tgt = mock_one;

	// Mock to serialize and import from proc 1 (not yet in the graph)
	importedArcMocks[1].push_back(mock);

	EXPECT_CALL(*localNode, linkOut(_)).Times(0);
	EXPECT_CALL(*localNode, linkIn(_));

	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

	node_type* setDistantArg;
	EXPECT_CALL(locationManager, setDistant(_))
		.WillOnce(SaveArg<0>(&setDistantArg));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distantNode = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(setDistantArg, distantNode);

	ASSERT_EQ(importedArc->tgt, localNode);
	ASSERT_EQ(importedArc->src, distantNode);
}

/********************/
/* distribute_tests */
/********************/
class BasicDistributedGraphDistributeTest : public BasicDistributedGraphTest {
	typedef typename graph_type::partition_type partition_type;

	protected:
		std::array<DistributedId, 5> nodeIds;
		partition_type fakePartition;
		MockLocationManager<node_type>& locationManager
			= const_cast<MockLocationManager<node_type>&>(
				static_cast<const MockLocationManager<node_type>&>(
					graph.getLocationManager()
					));
		void SetUp() override {
			BasicDistributedGraphTest::SetUp();

			EXPECT_CALL(locationManager, addManagedNode).Times(5);
			EXPECT_CALL(locationManager, setLocal).Times(5);
			EXPECT_CALL(graph.getSyncModeRuntime(), setUp).Times(5);
			for(int i=0; i < 5;i++) {
				auto node = graph.buildNode();
				nodeIds[i] = node->getId();
			}
			fakePartition[nodeIds[0]] = 7;
			fakePartition[nodeIds[1]] = 1;
			fakePartition[nodeIds[2]] = 1;
			fakePartition[nodeIds[3]] = 0;
			fakePartition[nodeIds[4]] = 0;
			EXPECT_CALL(mockSyncLinker, synchronize);
			EXPECT_CALL(mockDataSync, synchronize);
		}
};

/*
 * Check the correct call order of the synchronization steps of the
 * distribute() function.
 * 1. Synchronize links (eventually migrates link/unlink operations)
 * 2. Migrate node / arcs
 * 3. Synchronize Node locations
 * 4. Synchronize Node data
 */
TEST_F(BasicDistributedGraphTest, distribute_calls) {
	Sequence s;
	EXPECT_CALL(
			//(const_cast<MockSyncLinker<node_type, arc_type>&>(graph.getSyncLinker())),
			mockSyncLinker,
			synchronize()
			)
		.InSequence(s);

	EXPECT_CALL(
			comm, allToAll)
		.Times(AnyNumber())
		.InSequence(s);
	EXPECT_CALL(
			(const_cast<MockLocationManager<node_type>&>(
				static_cast<const MockLocationManager<node_type>&>(
						graph.getLocationManager()))),
			updateLocations(IsEmpty())
			)
		.InSequence(s);
	EXPECT_CALL(
			//(const_cast<MockDataSync<node_type, arc_type>&>(graph.getDataSync())),
			mockDataSync,
			synchronize()
			)
		.InSequence(s);

	graph.distribute({});
}

/*
 * Check the correct call order of the synchronization steps of the
 * synchronize() function.
 * 1. Synchronize links (eventually migrates link/unlink operations)
 * 4. Synchronize Node data
 */
TEST_F(BasicDistributedGraphTest, synchronize_calls) {
	Sequence s;
	EXPECT_CALL(
			//(const_cast<MockSyncLinker<node_type, arc_type>&>(graph.getSyncLinker())),
			mockSyncLinker,
			synchronize())
		.InSequence(s);
	EXPECT_CALL(
			//(const_cast<MockDataSync<node_type, arc_type>&>(graph.getDataSync())),
			mockDataSync,
			synchronize())
		.InSequence(s);

	graph.synchronize();
}

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
	//EXPECT_CALL(comm, allToAll(_)).Times(2);
	EXPECT_CALL(graph.getNodeMpi(), migrate);
	EXPECT_CALL(graph.getArcMpi(), migrate);

	EXPECT_CALL(locationManager, setDistant).Times(AnyNumber());
	EXPECT_CALL(locationManager, remove).Times(4);
	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));

	// Actual call
	graph.balance();
}

/*
 * Node distribution test (no arcs)
 */
TEST_F(BasicDistributedGraphDistributeTest, distribute_without_link) {
	auto exportMapMatcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					*graph.getNode(nodeIds[3]), *graph.getNode(nodeIds[4])
					)),
			Pair(1, UnorderedElementsAre(
					*graph.getNode(nodeIds[1]), *graph.getNode(nodeIds[2])
					))
			);

	// Mock node import
	std::unordered_map<int, std::vector<node_type>> nodeImport {
		{2, {
			node_type(DistributedId(2, 5)),
			node_type(DistributedId(4, 3))
			}}
	};

	EXPECT_CALL(
		const_cast<MockMpi<node_type>&>(graph.getNodeMpi()), migrate(exportMapMatcher))
		.WillOnce(Return(nodeImport));

	// No arc import
	EXPECT_CALL(
			const_cast<MockMpi<arc_type>&>(graph.getArcMpi()), migrate(IsEmpty()))
		.WillOnce(Return(std::unordered_map<int, std::vector<arc_type>>()));;

	std::array<node_type*, 4> removeArgs;
	EXPECT_CALL(locationManager, setDistant).Times(AnyNumber());
	EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[1])));
	EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[2])));
	EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[3])));
	EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[4])));

	std::array<node_type*, 2> setLocalArgs;
	EXPECT_CALL(locationManager, setLocal(_))
		.WillOnce(SaveArg<0>(&setLocalArgs[0]))
		.WillOnce(SaveArg<0>(&setLocalArgs[1]));
	EXPECT_CALL(graph.getSyncModeRuntime(), setUp).Times(2);
	
	EXPECT_CALL(locationManager, updateLocations(UnorderedElementsAre(
					Pair(DistributedId(2, 5), _),
					Pair(DistributedId(4, 3), _)
				)));
	graph.distribute(fakePartition);
	ASSERT_EQ(graph.getNodes().size(), 3);
	ASSERT_EQ(graph.getNodes().count(nodeIds[0]), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(2, 5)), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(4, 3)), 1);

	EXPECT_THAT(setLocalArgs, UnorderedElementsAre(
			graph.getNode(DistributedId(2, 5)),
			graph.getNode(DistributedId(4, 3))
			));
}

/****************************/
/* distribute_test_with_arc */
/****************************/
class BasicDistributedGraphDistributedWithLinkTest : public BasicDistributedGraphDistributeTest {
	protected:
		arc_type* arc1;
		arc_type* arc2;
		void SetUp() override {
			BasicDistributedGraphDistributeTest::SetUp();

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

			// Other uninteresting setDistant calls might occur on unconnected
			// nodes that will be cleared
			EXPECT_CALL(locationManager, setDistant).Times(AnyNumber());

			// Set linked nodes as distant nodes
			EXPECT_CALL(locationManager, setDistant(graph.getNode(nodeIds[2])));
			EXPECT_CALL(locationManager, setDistant(graph.getNode(nodeIds[3])));

			EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[1])));
			EXPECT_CALL(locationManager, remove(graph.getNode(nodeIds[4])));
		}
};

/*
 * Distribute nodes + arcs
 */
TEST_F(BasicDistributedGraphDistributedWithLinkTest, distribute_with_link_test) {
	auto exportNodeMapMatcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					*graph.getNode(nodeIds[3]), *graph.getNode(nodeIds[4])
					)),
			Pair(1, UnorderedElementsAre(
					*graph.getNode(nodeIds[1]), *graph.getNode(nodeIds[2])
					))
			);
	// No node import
	EXPECT_CALL(
		const_cast<MockMpi<node_type>&>(graph.getNodeMpi()), migrate(exportNodeMapMatcher));

	auto exportArcMapMatcher = UnorderedElementsAre(
		Pair(0, ElementsAre(*arc2)),
		Pair(1, ElementsAre(*arc1))
	);

	std::unordered_map<int, std::vector<arc_type>> arcImport {
		{3, {{DistributedId(4, 6), 2}}}
	};
	arcImport[3][0].src = new node_type(DistributedId(4, 8));
	arcImport[3][0].tgt = new node_type(DistributedId(3, 2));

	EXPECT_CALL(graph.getSyncModeRuntime(), setUp).Times(2);

	// Mock arc import
	EXPECT_CALL(
		const_cast<MockMpi<arc_type>&>(graph.getArcMpi()),
		migrate(exportArcMapMatcher))
		.WillOnce(Return(arcImport));

	EXPECT_CALL(locationManager, updateLocations(IsEmpty()));
	graph.distribute(fakePartition);

	ASSERT_EQ(graph.getArcs().size(), 3);
	ASSERT_EQ(graph.getArcs().count(DistributedId(4, 6)), 1);
	ASSERT_EQ(graph.getArcs().count(arc1->getId()), 1);
	ASSERT_EQ(graph.getArcs().count(arc2->getId()), 1);
}
