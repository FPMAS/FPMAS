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
#include "graph/parallel/distributed_graph.h"

#include "../mocks/communication/mock_communication.h"
#include "api/graph/base/graph.h"
#include "api/load_balancing/load_balancing.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/utils/mock_callback.h"


using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Expectation;
using ::testing::InvokeWithoutArgs;
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

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::NodePtrWrapper;
using FPMAS::graph::parallel::ArcPtrWrapper;

/********************/
/* local_build_node */
/********************/
class DistributedGraphTest : public ::testing::Test {
	protected:
		static const int CURRENT_RANK = 7;
		DistributedGraph<
			int,
			MockSyncMode<>,
			MockDistributedNode,
			MockDistributedArc,
			MockMpiSetUp<CURRENT_RANK, 10>,
			MockLocationManager
			> graph;

		typedef decltype(graph) GraphType;
		typedef MockDistributedNode<int> MockNode;
		typedef FPMAS::api::graph::parallel::DistributedNode<int> NodeType; 
		typedef MockDistributedArc<int> MockArc;
		typedef FPMAS::api::graph::parallel::DistributedArc<int> ArcType;
		typedef typename GraphType::SyncModeRuntimeType SyncModeRuntimeType;

		MockMpiCommunicator<CURRENT_RANK, 10>& comm =
			static_cast<MockMpiCommunicator<CURRENT_RANK, 10>&>(graph.getMpiCommunicator());

		MockLocationManager<int>& location_manager
			= const_cast<MockLocationManager<int>&>(
					graph.getLocationManager()
					);

		MockSyncLinker<int> mockSyncLinker;
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
TEST_F(DistributedGraphTest, build_node) {

	MockLocationManager<int>& location_manager
		= const_cast<MockLocationManager<int>&>(
				graph.getLocationManager()
				);
	auto local_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetLocal(local_callback);

	NodeType* add_managed_node_arg;
	EXPECT_CALL(location_manager, addManagedNode(_, CURRENT_RANK))
		.WillOnce(SaveArg<0>(&add_managed_node_arg));
	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call)
		.WillOnce(SaveArg<0>(&local_callback_arg));

	auto currentId = graph.currentNodeId();
	ASSERT_EQ(currentId.rank(), 7);

	MockMutex<int>* built_mutex = new MockMutex<int>;
	EXPECT_CALL(const_cast<SyncModeRuntimeType&>(graph.getSyncModeRuntime()), buildMutex(currentId, Eq(2)))
		.WillOnce(Return(built_mutex));

	auto node = graph.buildNode(2);
	ASSERT_EQ(add_managed_node_arg, node);
	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);

	ASSERT_EQ(graph.getNodes().count(currentId), 1);
	ASSERT_EQ(graph.getNode(currentId), node);
	ASSERT_EQ(node->getId(), currentId);

	ASSERT_EQ(node->state(), LocationState::LOCAL);

	ASSERT_EQ(&node->mutex(), built_mutex);
	ASSERT_EQ(node->data(), 2);
}

/**************/
/* local_link */
/**************/
class DistributedGraphLinkTest : public DistributedGraphTest {
	protected:
		MockNode* srcMock = new MockNode({0, 1});
		MockMutex<int> srcMutex;
		MockNode* tgtMock  = new MockNode({0, 2});
		MockMutex<int> tgtMutex;
		DistributedId currentId = graph.currentArcId();
		ArcType* linkOutArg;
		ArcType* linkInArg;

	public:
		void SetUp() override {
			DistributedGraphTest::SetUp();

			graph.insert(srcMock);
			graph.insert(tgtMock);

			EXPECT_CALL(*srcMock, mutex()).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(Const(*srcMock), mutex()).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(*tgtMock, mutex()).WillRepeatedly(ReturnRef(tgtMutex));
			EXPECT_CALL(Const(*tgtMock), mutex()).WillRepeatedly(ReturnRef(tgtMutex));

			EXPECT_CALL(srcMutex, lock);
			EXPECT_CALL(srcMutex, unlock);
			EXPECT_CALL(tgtMutex, lock);
			EXPECT_CALL(tgtMutex, unlock);
			EXPECT_CALL(mockSyncLinker, unlink).Times(0);

			EXPECT_CALL(*srcMock, linkOut)
				.WillOnce(SaveArg<0>(&linkOutArg));
			EXPECT_CALL(*srcMock, unlinkOut);
			EXPECT_CALL(*tgtMock, linkIn)
				.WillOnce(SaveArg<0>(&linkInArg));
			EXPECT_CALL(*tgtMock, unlinkIn);
		}

		void checkArcStructure(MockArc* arc) {
			ASSERT_EQ(linkInArg, arc);
			ASSERT_EQ(linkOutArg, arc);

			ASSERT_EQ(graph.getArcs().count(currentId), 1);
			ASSERT_EQ(graph.getArc(currentId), arc);
			ASSERT_EQ(arc->getId(), currentId);

			ASSERT_EQ(arc->src, srcMock);
			ASSERT_EQ(arc->tgt, tgtMock);

			EXPECT_CALL(*arc, getLayer);
			ASSERT_EQ(arc->getLayer(), 14);
		}
	};
/*
 * Local link
 */
TEST_F(DistributedGraphLinkTest, local_src_local_tgt) {
	EXPECT_CALL(*srcMock, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(*tgtMock, state).WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(mockSyncLinker, link);

	auto arc = graph.link(srcMock, tgtMock, 14);

	checkArcStructure(static_cast<MockArc*>(arc));

	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::LOCAL);
}

/*
 * Local src, distant tgt
 */
TEST_F(DistributedGraphLinkTest, local_src_distant_tgt) {
	EXPECT_CALL(*srcMock, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(*tgtMock, state).WillRepeatedly(Return(LocationState::DISTANT));

	const ArcType* linkArcArg;
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = graph.link(srcMock, tgtMock, 14);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(static_cast<MockArc*>(arc));

	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::DISTANT);
}

/*
 * Distant src, local tgt
 */
TEST_F(DistributedGraphLinkTest, distant_src_local_tgt) {
	EXPECT_CALL(*srcMock, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(*tgtMock, state).WillRepeatedly(Return(LocationState::LOCAL));

	const ArcType* linkArcArg;
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = graph.link(srcMock, tgtMock, 14);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(static_cast<MockArc*>(arc));

	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::DISTANT);
}

/*
 * Distant src, distant tgt
 */
// TODO : what should we do with such an arc? Should it be deleted once is has
// been sent?
TEST_F(DistributedGraphLinkTest, distant_src_distant_tgt) {
	EXPECT_CALL(*srcMock, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(*tgtMock, state).WillRepeatedly(Return(LocationState::DISTANT));

	const ArcType* linkArcArg;
	EXPECT_CALL(mockSyncLinker, link)
		.WillOnce(SaveArg<0>(&linkArcArg));
	auto arc = graph.link(srcMock, tgtMock, 14);
	ASSERT_EQ(linkArcArg, arc);

	checkArcStructure(static_cast<MockArc*>(arc));

	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::DISTANT);
}

/****************/
/* unlink_tests */
/****************/
class DistributedGraphUnlinkTest : public DistributedGraphTest {
	protected:
		MockNode* srcMock = new MockNode(DistributedId(0, 0));
		MockMutex<int> srcMutex;
		MockNode* tgtMock = new MockNode(DistributedId(0, 1));
		MockMutex<int> tgtMutex;
		ArcType* arc;

	public:
		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(*srcMock, mutex()).WillRepeatedly(ReturnRef(srcMutex));
			EXPECT_CALL(*tgtMock, mutex()).WillRepeatedly(ReturnRef(tgtMutex));

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

TEST_F(DistributedGraphUnlinkTest, local_src_local_tgt) {
	this->link(LocationState::LOCAL, LocationState::LOCAL);
	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::LOCAL);

	EXPECT_CALL(mockSyncLinker, unlink);

	graph.unlink(arc);

	ASSERT_EQ(graph.getArcs().size(), 0);

	delete srcMock;
	delete tgtMock;
}

TEST_F(DistributedGraphUnlinkTest, local_src_distant_tgt) {
	this->link(LocationState::LOCAL, LocationState::DISTANT);
	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::DISTANT);

	EXPECT_CALL(mockSyncLinker, unlink(arc));
	EXPECT_CALL(location_manager, remove(tgtMock));

	graph.unlink(arc);

	ASSERT_EQ(graph.getArcs().size(), 0);
	delete srcMock;
}

TEST_F(DistributedGraphUnlinkTest, distant_src_local_tgt) {
	this->link(LocationState::DISTANT, LocationState::LOCAL);
	ASSERT_EQ(static_cast<MockArc*>(arc)->_state, LocationState::DISTANT);

	EXPECT_CALL(mockSyncLinker, unlink(static_cast<MockArc*>(arc)));
	EXPECT_CALL(location_manager, remove(srcMock));

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
class DistributedGraphImportNodeTest : public DistributedGraphTest {
	protected:
		typedef typename GraphType::NodeType NodeType; // MockDistributedNode<int, MockMutex>
		typedef typename GraphType::ArcType ArcType; // MockDistributedArc<int, MockMutex>
		typedef typename decltype(graph)::PartitionMap PartitionMap;

		PartitionMap partition;
		std::unordered_map<int, std::vector<NodePtrWrapper<int>>> imported_node_mocks;
		std::unordered_map<int, std::vector<ArcPtrWrapper<int>>> imported_arc_mocks;
		
		void SetUp() override {
			DistributedGraphTest::SetUp();
		}

		void distributeTest() {
			EXPECT_CALL(
					(const_cast<MockMpi<NodePtrWrapper<int>>&>(graph.getNodeMpi())),
					migrate
					).WillOnce(Return(imported_node_mocks));
			EXPECT_CALL(
					(const_cast<MockMpi<ArcPtrWrapper<int>>&>(graph.getArcMpi())),
					migrate
					).WillOnce(Return(imported_arc_mocks));
			EXPECT_CALL(mockSyncLinker, synchronize).Times(1);
			EXPECT_CALL(mockDataSync, synchronize).Times(1);
			graph.distribute(partition);
		}

};

/*
 * Import new node test
 */
TEST_F(DistributedGraphImportNodeTest, import_node) {
	imported_node_mocks[1].emplace_back(new MockNode(DistributedId(1, 10), 8, 2.1));
	auto local_nodes_matcher = ElementsAre(
		Key(DistributedId(1, 10))
		);

	EXPECT_CALL(location_manager,updateLocations(local_nodes_matcher));

	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));
	EXPECT_CALL(
		const_cast<SyncModeRuntimeType&>(graph.getSyncModeRuntime()),
		buildMutex(DistributedId(1, 10), Eq(8))
		);

	// Callback call test
	auto local_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetLocal(local_callback);
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call)
		.WillOnce(SaveArg<0>(&local_callback_arg));

	distributeTest();

	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(1, 10)), 1);
	auto node = graph.getNode(DistributedId(1, 10));
	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);
	ASSERT_EQ(node->data(), 8);
	ASSERT_EQ(node->getWeight(), 2.1f);
}

/*
 * Import node with existing ghost test
 */
TEST_F(DistributedGraphImportNodeTest, import_node_with_existing_ghost) {
	EXPECT_CALL(location_manager, addManagedNode);
	EXPECT_CALL(location_manager, setLocal);
	EXPECT_CALL(
		const_cast<SyncModeRuntimeType&>(graph.getSyncModeRuntime()),
		buildMutex(graph.currentNodeId(), Eq(8))
		);
	auto node = graph.buildNode(8);
	ON_CALL(*static_cast<MockNode*>(node), state()).WillByDefault(Return(LocationState::DISTANT));

	imported_node_mocks[1].emplace_back(new MockNode(node->getId(), 2, 4.));
	auto local_nodes_matcher = ElementsAre(
			Key(node->getId())
			);

	EXPECT_CALL(location_manager, updateLocations(local_nodes_matcher));

	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));

	// Callback call test
	auto local_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetLocal(local_callback);
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call)
		.WillOnce(SaveArg<0>(&local_callback_arg));

	distributeTest();

	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);
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
class DistributedGraphImportArcTest : public DistributedGraphImportNodeTest {
	protected:
		NodeType* src;
		NodeType* tgt;
		MockNode* mock_src;
		MockNode* mock_tgt;

		ArcType* imported_arc;
		MockArc* mock_arc;

	void SetUp() override {
		DistributedGraphImportNodeTest::SetUp();

		EXPECT_CALL(location_manager, addManagedNode).Times(AnyNumber());
		EXPECT_CALL(location_manager, setLocal).Times(AnyNumber());
		EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(AnyNumber());
		src = graph.buildNode();
		tgt = graph.buildNode();

		mock_src = new MockNode(src->getId());
		mock_tgt = new MockNode(tgt->getId());
		mock_arc = new MockArc(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);
		mock_arc->src = mock_src;
		mock_arc->tgt = mock_tgt;

		// Mock to serialize and import from proc 1 (not yet in the graph)
		imported_arc_mocks[1].push_back(mock_arc);

		EXPECT_CALL(*static_cast<MockNode*>(src), linkOut(_));
		EXPECT_CALL(*static_cast<MockNode*>(src), unlinkOut(_));
		EXPECT_CALL(*static_cast<MockNode*>(tgt), linkIn(_));
		EXPECT_CALL(*static_cast<MockNode*>(tgt), unlinkIn(_));
	}

	void checkArcStructure(DistributedId id = DistributedId(3, 12)) {
		ASSERT_EQ(graph.getArcs().size(), 1);
		ASSERT_EQ(graph.getArcs().count(id), 1);
		imported_arc = graph.getArc(id);
		ASSERT_EQ(imported_arc->getLayer(), 2);

		ASSERT_EQ(static_cast<MockArc*>(imported_arc)->src, src);
		ASSERT_EQ(static_cast<MockArc*>(imported_arc)->tgt, tgt);
	}

};

/*
 * Import an arc already contained in the graph, as a distant arc.
 * Nothing is done, but the operation can occur when migrating arcs : a proc
 * can send an arc to an other proc, because it is linked to an exported node,
 * but the arcs represented on the target node are not known. In consequence,
 * the imported arc might have already been imported with an other node in a
 * previous iteration.
 */
TEST_F(DistributedGraphImportArcTest, import_existing_local_arc) {
	EXPECT_CALL(*static_cast<MockNode*>(src), state)
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockSyncLinker, link);

	DistributedId id;
	auto* mocked_existing_arc = graph.link(src, tgt, 2);
	EXPECT_CALL(*static_cast<MockArc*>(mocked_existing_arc), state)
		.WillRepeatedly(Return(LocationState::DISTANT));
	id = mocked_existing_arc->getId();

	ON_CALL(*mock_arc, getId).WillByDefault(Return(id));

	graph.importArc(mock_arc);

	checkArcStructure(id);
	ASSERT_EQ(static_cast<MockArc*>(imported_arc)->_state, LocationState::DISTANT);
}

/*
 * Import arc when the two nodes are LOCAL
 */
TEST_F(DistributedGraphImportArcTest, import_local_arc) {
	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	distributeTest();
	checkArcStructure();
	ASSERT_EQ(static_cast<MockArc*>(imported_arc)->_state, LocationState::LOCAL);
}


/*
 * Import with DISTANT src
 */
TEST_F(DistributedGraphImportArcTest, import_arc_with_existing_distant_src) {
	EXPECT_CALL(*static_cast<MockNode*>(src), state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	distributeTest();
	checkArcStructure();
	ASSERT_EQ(static_cast<MockArc*>(imported_arc)->_state, LocationState::DISTANT);
}

/*
 * Import with DISTANT tgt
 */
TEST_F(DistributedGraphImportArcTest, import_arc_with_existing_distant_tgt) {
	EXPECT_CALL(*static_cast<MockNode*>(tgt), state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	distributeTest();
	checkArcStructure();
	ASSERT_EQ(static_cast<MockArc*>(imported_arc)->_state, LocationState::DISTANT);
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
TEST_F(DistributedGraphImportArcTest, double_import_edge_case) {
	auto mock = new MockArc(DistributedId(3, 12), 2, 2.6);

	mock->src = new MockNode(src->getId());
	mock->tgt = new MockNode(tgt->getId());

	// The same arc is imported from proc 2
	imported_arc_mocks[2].push_back(mock);
	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));
	distributeTest();
	checkArcStructure();
	ASSERT_EQ(static_cast<MockArc*>(imported_arc)->_state, LocationState::LOCAL);
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
class DistributedGraphImportArcWithGhostTest : public DistributedGraphImportNodeTest {
	protected:
		NodeType* local_node;
		MockNode* mock_one;
		MockNode* mock_two;

		ArcType* importedArc;

		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex);
			EXPECT_CALL(location_manager, addManagedNode);
			EXPECT_CALL(location_manager, setLocal);
			local_node = graph.buildNode();

			mock_one = new MockNode(local_node->getId());
			mock_two = new MockNode(DistributedId(8, 16));

			// This node is not contained is the graph, but will be linked.
			ASSERT_EQ(graph.getNodes().count(mock_two->getId()), 0);

			// SetUp should be call once for the created tgt or src
			EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex);
		}

		void checkArcAndGhostStructure() {
			ASSERT_EQ(graph.getArcs().size(), 1);
			ASSERT_EQ(graph.getArcs().count(DistributedId(3, 12)), 1);
			importedArc = graph.getArc(DistributedId(3, 12));
			ASSERT_EQ(importedArc->getLayer(), 2);
			ASSERT_EQ(importedArc->getWeight(), 2.6f);
			ASSERT_EQ(static_cast<MockArc*>(importedArc)->_state, LocationState::DISTANT);

			EXPECT_EQ(graph.getNodes().size(), 2);
			EXPECT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
			auto ghostNode = graph.getNode(DistributedId(8, 16));
			EXPECT_EQ(ghostNode->getId(), DistributedId(8, 16));
		}
};

/*
 * LOCAL src, missing tgt
 */
TEST_F(DistributedGraphImportArcWithGhostTest, import_with_missing_tgt) {
	auto mock = new MockArc(DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock->src = mock_one;
	mock->tgt = mock_two; // This node is not yet contained in the graph

	// Mock to serialize and import from proc 1 (not yet in the graph)
	imported_arc_mocks[1].push_back(mock);

	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkIn(_)).Times(0);
	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkOut(_));
	EXPECT_CALL(*static_cast<MockNode*>(local_node), unlinkOut);
	EXPECT_CALL(*mock_two, unlinkIn);


	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	NodeType* set_distant_arg;
	EXPECT_CALL(location_manager, setDistant(_))
		.WillOnce(SaveArg<0>(&set_distant_arg));

	// Distant call back test
	auto* distant_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetDistant(distant_callback);
	NodeType* distant_callback_arg;
	EXPECT_CALL(*distant_callback, call)
		.WillOnce(SaveArg<0>(&distant_callback_arg));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockArc*>(importedArc)->src, local_node);
	ASSERT_EQ(static_cast<MockArc*>(importedArc)->tgt, distant_node);
}

/*
 * Missing src, LOCAL tgt
 */
TEST_F(DistributedGraphImportArcWithGhostTest, import_with_missing_src) {
	auto mock = new MockArc(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock->src = mock_two; // This node is not yet contained in the graph
	mock->tgt = mock_one;

	// Mock to serialize and import from proc 1 (not yet in the graph)
	imported_arc_mocks[1].push_back(mock);

	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkOut(_)).Times(0);
	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkIn(_));
	EXPECT_CALL(*static_cast<MockNode*>(local_node), unlinkIn);
	EXPECT_CALL(*mock_two, unlinkOut);

	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	NodeType* set_distant_arg;
	EXPECT_CALL(location_manager, setDistant(_))
		.WillOnce(SaveArg<0>(&set_distant_arg));
	// Distant call back test
	auto* distant_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetDistant(distant_callback);
	NodeType* distant_callback_arg;
	EXPECT_CALL(*distant_callback, call)
		.WillOnce(SaveArg<0>(&distant_callback_arg));

	distributeTest();
	checkArcAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockArc*>(importedArc)->tgt, local_node);
	ASSERT_EQ(static_cast<MockArc*>(importedArc)->src, distant_node);
}

/********************/
/* distribute_tests */
/********************/
class DistributedGraphDistributeTest : public DistributedGraphTest {
	typedef typename GraphType::PartitionMap PartitionMap;

	protected:
		std::array<DistributedId, 5> nodeIds;
		PartitionMap fakePartition;
		MockLocationManager<int>& location_manager
			= const_cast<MockLocationManager<int>&>(
				static_cast<const MockLocationManager<int>&>(
					graph.getLocationManager()
					));
		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(location_manager, addManagedNode).Times(5);
			EXPECT_CALL(location_manager, setLocal).Times(5);
			EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(5);
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
TEST_F(DistributedGraphTest, distribute_calls) {
	Sequence s;
	EXPECT_CALL(
			//(const_cast<MockSyncLinker<NodeType, ArcType>&>(graph.getSyncLinker())),
			mockSyncLinker,
			synchronize()
			)
		.InSequence(s);

	EXPECT_CALL(
			comm, allToAll)
		.Times(AnyNumber())
		.InSequence(s);
	EXPECT_CALL(
			(const_cast<MockLocationManager<int>&>(
				static_cast<const MockLocationManager<int>&>(
						graph.getLocationManager()))),
			updateLocations(IsEmpty())
			)
		.InSequence(s);
	EXPECT_CALL(
			//(const_cast<MockDataSync<NodeType, ArcType>&>(graph.getDataSync())),
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
TEST_F(DistributedGraphTest, synchronize_calls) {
	Sequence s;
	EXPECT_CALL(
			//(const_cast<MockSyncLinker<NodeType, ArcType>&>(graph.getSyncLinker())),
			mockSyncLinker,
			synchronize())
		.InSequence(s);
	EXPECT_CALL(
			//(const_cast<MockDataSync<NodeType, ArcType>&>(graph.getDataSync())),
			mockDataSync,
			synchronize())
		.InSequence(s);

	graph.synchronize();
}

/*
 * Balance test.
 * Checks that load balancing is called and migration is performed.
 */
TEST_F(DistributedGraphDistributeTest, balance) {
	MockLoadBalancing<int> load_balancing;
	typename MockLoadBalancing<int>::ConstNodeMap node_map;
	for(auto node : graph.getNodes())
		node_map.insert(node);

	EXPECT_CALL(location_manager, getLocalNodes).WillOnce(ReturnRef(graph.getNodes()));

	// Should call LoadBalancing on all nodes, without fixed nodes
	EXPECT_CALL(
		load_balancing,
		balance(node_map))
		.WillOnce(Return(fakePartition));
	// Migration of nodes + arcs
	EXPECT_CALL(graph.getNodeMpi(), migrate);
	EXPECT_CALL(graph.getArcMpi(), migrate);

	EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());
	EXPECT_CALL(location_manager, remove).Times(4);
	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));

	// Actual call
	graph.balance(load_balancing);
}

/*
 * Node distribution test (no arcs)
 */
TEST_F(DistributedGraphDistributeTest, distribute_without_link) {
	auto export_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					graph.getNode(nodeIds[3]),
					graph.getNode(nodeIds[4])
					)),
			Pair(1, UnorderedElementsAre(
					graph.getNode(nodeIds[1]),
					graph.getNode(nodeIds[2])
					))
			);

	// Mock node import
	std::unordered_map<int, std::vector<NodePtrWrapper<int>>> node_import {
		{2, {
			new MockNode(DistributedId(2, 5)),
			new MockNode(DistributedId(4, 3))
			}}
	};

	EXPECT_CALL(
		const_cast<MockMpi<NodePtrWrapper<int>>&>(graph.getNodeMpi()), migrate(export_map_matcher))
		.WillOnce(Return(node_import));

	// No arc import
	EXPECT_CALL(
			const_cast<MockMpi<ArcPtrWrapper<int>>&>(graph.getArcMpi()), migrate(IsEmpty()))
		.WillOnce(Return(std::unordered_map<int, std::vector<ArcPtrWrapper<int>>>()));;

	EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());
	EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[1])));
	EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[2])));
	EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[3])));
	EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[4])));

	std::array<NodeType*, 2> set_local_args;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_args[0]))
		.WillOnce(SaveArg<0>(&set_local_args[1]));
	EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(2);
	
	EXPECT_CALL(location_manager, updateLocations(UnorderedElementsAre(
					Pair(DistributedId(2, 5), _),
					Pair(DistributedId(4, 3), _)
				)));
	graph.distribute(fakePartition);
	ASSERT_EQ(graph.getNodes().size(), 3);
	ASSERT_EQ(graph.getNodes().count(nodeIds[0]), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(2, 5)), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(4, 3)), 1);

	EXPECT_THAT(set_local_args, UnorderedElementsAre(
			graph.getNode(DistributedId(2, 5)),
			graph.getNode(DistributedId(4, 3))
			));
}

/****************************/
/* distribute_test_with_arc */
/****************************/
class DistributedGraphDistributeWithLinkTest : public DistributedGraphDistributeTest {
	protected:
		typedef std::vector<FPMAS::api::graph::parallel::DistributedArc<int>*> ArcList;

		ArcType* arc1;
		ArcType* arc2;

		ArcList* node2_in = new ArcList;
		ArcList* node3_out = new ArcList;

		MockCallback<NodeType*>* distant_callback = new MockCallback<NodeType*>;
		std::array<NodeType*, 2> imported_arc_distant_callback_args;

		void SetUp() override {
			DistributedGraphDistributeTest::SetUp();

			// No lock to manage, all links are local
			MockMutex<int> mockMutex;
			EXPECT_CALL(mockMutex, lock).Times(AnyNumber());
			EXPECT_CALL(mockMutex, unlock).Times(AnyNumber());
			EXPECT_CALL(mockSyncLinker, link).Times(2);

			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[0])), mutex())
				.WillRepeatedly(ReturnRef(mockMutex));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[2])), mutex())
				.WillRepeatedly(ReturnRef(mockMutex));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[3])), mutex())
				.WillRepeatedly(ReturnRef(mockMutex));

			arc1 = graph.link(graph.getNode(nodeIds[0]), graph.getNode(nodeIds[2]), 0);
			node2_in->push_back(arc1);
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[2])), getIncomingArcs())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(node2_in));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[2])), unlinkIn)
				.WillOnce(InvokeWithoutArgs(node2_in, &ArcList::clear));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[0])), unlinkOut);


			arc2 = graph.link(graph.getNode(nodeIds[3]), graph.getNode(nodeIds[0]), 0);
			node3_out->push_back(arc2);
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[3])), getOutgoingArcs())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(node3_out));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[3])), unlinkOut)
				.WillOnce(InvokeWithoutArgs(node3_out, &ArcList::clear));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(nodeIds[0])), unlinkIn);

			// Other uninteresting setDistant calls might occur on unconnected
			// nodes that will be cleared
			EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());

			graph.addCallOnSetDistant(distant_callback);

			// Those two expectation are matched only when the next 4
			// expectations are not matched.
			EXPECT_CALL(*distant_callback, call)
				.WillOnce(SaveArg<0>(&imported_arc_distant_callback_args[0]))
				.WillOnce(SaveArg<0>(&imported_arc_distant_callback_args[1]));

			// Exported node callback matchers
			EXPECT_CALL(*distant_callback, call(graph.getNode(nodeIds[2])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(nodeIds[3])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(nodeIds[1])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(nodeIds[4])));

			// Set linked nodes as distant nodes
			EXPECT_CALL(location_manager, setDistant(graph.getNode(nodeIds[2])));
			EXPECT_CALL(location_manager, setDistant(graph.getNode(nodeIds[3])));

			EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[1])));
			EXPECT_CALL(location_manager, remove(graph.getNode(nodeIds[4])));
		}

		void TearDown() override {
			delete node2_in;
			delete node3_out;
		}
};

/*
 * Distribute nodes + arcs
 */
TEST_F(DistributedGraphDistributeWithLinkTest, distribute_with_link_test) {
	auto export_node_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					graph.getNode(nodeIds[3]),
					graph.getNode(nodeIds[4])
					)),
			Pair(1, UnorderedElementsAre(
					graph.getNode(nodeIds[1]),
					graph.getNode(nodeIds[2])
					))
			);
	// No node import
	EXPECT_CALL(
		const_cast<MockMpi<NodePtrWrapper<int>>&>(graph.getNodeMpi()), migrate(export_node_map_matcher));

	auto export_arc_map_matcher = UnorderedElementsAre(
		Pair(0, ElementsAre(arc2)),
		Pair(1, ElementsAre(arc1))
	);

	auto arc = new MockArc(DistributedId(4, 6), 2);
	std::unordered_map<int, std::vector<ArcPtrWrapper<int>>> arc_import {
		{3, {arc}}
	};
	auto mock_src = new MockNode(DistributedId(4, 8));
	auto mock_tgt = new MockNode(DistributedId(3, 2));
	arc->src = mock_src;
	arc->tgt = mock_tgt;
	EXPECT_CALL(*mock_src, unlinkOut);
	EXPECT_CALL(*mock_tgt, unlinkIn);

	EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(2);

	// Mock arc import
	EXPECT_CALL(
		const_cast<MockMpi<ArcPtrWrapper<int>>&>(graph.getArcMpi()),
		migrate(export_arc_map_matcher))
		.WillOnce(Return(arc_import));

	EXPECT_CALL(location_manager, updateLocations(IsEmpty()));
	graph.distribute(fakePartition);

	ASSERT_THAT(imported_arc_distant_callback_args, UnorderedElementsAre(mock_src, mock_tgt));
	ASSERT_EQ(graph.getArcs().size(), 3);
	ASSERT_EQ(graph.getArcs().count(DistributedId(4, 6)), 1);
	ASSERT_EQ(graph.getArcs().count(arc1->getId()), 1);
	ASSERT_EQ(graph.getArcs().count(arc2->getId()), 1);
}
