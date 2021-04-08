/*********/
/* Index */
/*********/
/*
 * # local_build_node
 * # link_tests
 * # unlink_tests
 * # remove_node_tests
 * # import_node_tests
 * # import_edge_tests
 * # import_edge_with_missing_node_test
 * # insert_distant_test
 * # distribute_tests
 * # distribute_test_with_edge
 * # distribute_test_real_MPI
 */
#include "fpmas/graph/distributed_graph.h"

#include "../mocks/communication/mock_communication.h"
#include "fpmas/api/graph/graph.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/random/random.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/graph/mock_distributed_edge.h"
#include "../mocks/graph/mock_location_manager.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/graph/mock_load_balancing.h"
#include "../mocks/utils/mock_callback.h"


using namespace testing;

using fpmas::graph::detail::DistributedGraph;
using fpmas::graph::NodePtrWrapper;
using fpmas::graph::EdgePtrWrapper;

/********************/
/* local_build_node */
/********************/
class DistributedGraphTest : public Test {
	protected:
		static const int CURRENT_RANK = 7;
		MockMpiCommunicator<CURRENT_RANK, 10> comm;
		template<typename T>
			using MockDistributedNode = MockDistributedNode<T, NiceMock>;
		template<typename T>
			using MockDistributedEdge = MockDistributedEdge<T, NiceMock>;
		template<typename T>
			using MockSyncMode = NiceMock<MockSyncMode<T>>;
		DistributedGraph<
			int,
			MockSyncMode,
			MockDistributedNode,
			MockDistributedEdge,
			MockMpi,
			MockLocationManager
			> graph {comm};

		typedef decltype(graph) GraphType;
		typedef MockDistributedNode<int> MockNode;
		typedef fpmas::api::graph::DistributedNode<int> NodeType; 
		typedef MockDistributedEdge<int> MockEdge;
		typedef fpmas::api::graph::DistributedEdge<int> EdgeType;
		typedef typename GraphType::NodeMap NodeMap;

		MockLocationManager<int>& location_manager
			= const_cast<MockLocationManager<int>&>(
					graph.getLocationManager()
					);

		MockSyncLinker<int> mock_sync_linker;
		MockDataSync mock_data_sync;

	
		void SetUp() override {
			ON_CALL(graph.getSyncMode(), getSyncLinker)
				.WillByDefault(ReturnRef(mock_sync_linker));
			ON_CALL(graph.getSyncMode(), getDataSync)
				.WillByDefault(ReturnRef(mock_data_sync));
		}

};

/*
 * Local buildNode test
 */
TEST_F(DistributedGraphTest, build_node) {
	auto local_callback = new MockCallback<NodeType*>;
	graph.addCallOnSetLocal(local_callback);

	DistributedId add_managed_node_arg;
	EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), CURRENT_RANK))
		.WillOnce(SaveArg<0>(&add_managed_node_arg));
	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call)
		.WillOnce(SaveArg<0>(&local_callback_arg));

	auto currentId = graph.currentNodeId();
	ASSERT_EQ(currentId.rank(), 7);

	NodeType* build_mutex_arg;
	MockMutex<int>* built_mutex = new MockMutex<int>;
	EXPECT_CALL(const_cast<MockSyncMode<int>&>(graph.getSyncMode()), buildMutex)
		.WillOnce(DoAll(SaveArg<0>(&build_mutex_arg), Return(built_mutex)));

	auto node = graph.buildNode(2);
	ASSERT_EQ(build_mutex_arg, node);
	ASSERT_EQ(add_managed_node_arg, node->getId());
	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);

	ASSERT_EQ(graph.getNodes().count(currentId), 1);
	ASSERT_EQ(graph.getNode(currentId), node);
	ASSERT_EQ(node->getId(), currentId);

	ASSERT_EQ(node->state(), LocationState::LOCAL);

	ASSERT_EQ(node->mutex(), built_mutex);
	ASSERT_EQ(node->data(), 2);
}

/**************/
/* link_tests */
/**************/
class DistributedGraphLinkTest : public DistributedGraphTest {
	protected:
		MockNode* src_mock = new MockNode({0, 1});
		MockMutex<int> src_mutex;
		MockNode* tgt_mock  = new MockNode({0, 2});
		MockMutex<int> tgt_mutex;
		DistributedId currentId = graph.currentEdgeId();
		EdgeType* linkOutArg;
		EdgeType* linkInArg;

	public:
		void SetUp() override {
			DistributedGraphTest::SetUp();

			graph.insert(src_mock);
			graph.insert(tgt_mock);

			ON_CALL(*src_mock, mutex())
				.WillByDefault(Return(&src_mutex));
			ON_CALL(Const(*src_mock), mutex())
				.WillByDefault(Return(&src_mutex));
			ON_CALL(*tgt_mock, mutex())
				.WillByDefault(Return(&tgt_mutex));
			ON_CALL(Const(*tgt_mock), mutex())
				.WillByDefault(Return(&tgt_mutex));

			EXPECT_CALL(src_mutex, lockShared);
			EXPECT_CALL(src_mutex, unlockShared);
			EXPECT_CALL(tgt_mutex, lockShared);
			EXPECT_CALL(tgt_mutex, unlockShared);
			EXPECT_CALL(mock_sync_linker, unlink).Times(0);

			EXPECT_CALL(*src_mock, linkOut)
				.WillOnce(SaveArg<0>(&linkOutArg));
			EXPECT_CALL(*src_mock, unlinkOut);
			EXPECT_CALL(*tgt_mock, linkIn)
				.WillOnce(SaveArg<0>(&linkInArg));
			EXPECT_CALL(*tgt_mock, unlinkIn);
		}

		void checkEdgeStructure(MockEdge* edge) {
			ASSERT_EQ(linkInArg, edge);
			ASSERT_EQ(linkOutArg, edge);

			ASSERT_EQ(graph.getEdges().count(currentId), 1);
			ASSERT_EQ(graph.getEdge(currentId), edge);
			ASSERT_EQ(edge->getId(), currentId);

			ASSERT_EQ(edge->src, src_mock);
			ASSERT_EQ(edge->tgt, tgt_mock);

			EXPECT_CALL(*edge, getLayer);
			ASSERT_EQ(edge->getLayer(), 14);
		}
	};
/*
 * Local link
 */
TEST_F(DistributedGraphLinkTest, local_src_local_tgt) {
	ON_CALL(*src_mock, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(*tgt_mock, state)
		.WillByDefault(Return(LocationState::LOCAL));

	EXPECT_CALL(mock_sync_linker, link);

	auto edge = graph.link(src_mock, tgt_mock, 14);

	checkEdgeStructure(static_cast<MockEdge*>(edge));

	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::LOCAL);
}

/*
 * Local src, distant tgt
 */
TEST_F(DistributedGraphLinkTest, local_src_distant_tgt) {
	ON_CALL(*src_mock, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(*tgt_mock, state)
		.WillByDefault(Return(LocationState::DISTANT));

	const EdgeType* link_edge_arg;
	EXPECT_CALL(mock_sync_linker, link)
		.WillOnce(SaveArg<0>(&link_edge_arg));
	auto edge = graph.link(src_mock, tgt_mock, 14);
	ASSERT_EQ(link_edge_arg, edge);

	checkEdgeStructure(static_cast<MockEdge*>(edge));

	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::DISTANT);
}

/*
 * Distant src, local tgt
 */
TEST_F(DistributedGraphLinkTest, distant_src_local_tgt) {
	ON_CALL(*src_mock, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(*tgt_mock, state)
		.WillByDefault(Return(LocationState::LOCAL));

	const EdgeType* linkEdgeArg;
	EXPECT_CALL(mock_sync_linker, link)
		.WillOnce(SaveArg<0>(&linkEdgeArg));
	auto edge = graph.link(src_mock, tgt_mock, 14);
	ASSERT_EQ(linkEdgeArg, edge);

	checkEdgeStructure(static_cast<MockEdge*>(edge));

	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::DISTANT);
}

/*
 * Distant src, distant tgt
 */
// TODO : what should we do with such an edge? Should it be deleted once is has
// been sent?
TEST_F(DistributedGraphLinkTest, distant_src_distant_tgt) {
	ON_CALL(*src_mock, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(*tgt_mock, state)
		.WillByDefault(Return(LocationState::DISTANT));

	const EdgeType* linkEdgeArg;
	EXPECT_CALL(mock_sync_linker, link)
		.WillOnce(SaveArg<0>(&linkEdgeArg));
	auto edge = graph.link(src_mock, tgt_mock, 14);
	ASSERT_EQ(linkEdgeArg, edge);

	checkEdgeStructure(static_cast<MockEdge*>(edge));

	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::DISTANT);
}

/****************/
/* unlink_tests */
/****************/
class DistributedGraphUnlinkTest : public DistributedGraphTest {
	protected:
		MockNode* src_mock = new MockNode(DistributedId(0, 0));
		MockMutex<int> src_mutex;
		MockNode* tgt_mock = new MockNode(DistributedId(0, 1));
		MockMutex<int> tgt_mutex;
		EdgeType* edge;

	public:
		void SetUp() override {
			DistributedGraphTest::SetUp();

			ON_CALL(*src_mock, mutex())
				.WillByDefault(Return(&src_mutex));
			ON_CALL(*tgt_mock, mutex())
				.WillByDefault(Return(&tgt_mutex));

			EXPECT_CALL(mock_sync_linker, link).Times(AnyNumber());
		}

		void link(LocationState srcState, LocationState tgtState) {
			ON_CALL(*src_mock, state)
				.WillByDefault(Return(srcState));
			ON_CALL(*tgt_mock, state)
				.WillByDefault(Return(tgtState));

			EXPECT_CALL(*src_mock, linkOut);
			EXPECT_CALL(*tgt_mock, linkIn);

			edge = graph.link(src_mock, tgt_mock, 0);

			EXPECT_CALL(*src_mock, unlinkOut(edge));
			EXPECT_CALL(*tgt_mock, unlinkIn(edge));
		}
};

TEST_F(DistributedGraphUnlinkTest, local_src_local_tgt) {
	this->link(LocationState::LOCAL, LocationState::LOCAL);
	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::LOCAL);

	EXPECT_CALL(mock_sync_linker, unlink);

	graph.unlink(edge);

	ASSERT_EQ(graph.getEdges().size(), 0);

	delete src_mock;
	delete tgt_mock;
}

TEST_F(DistributedGraphUnlinkTest, local_src_distant_tgt) {
	this->link(LocationState::LOCAL, LocationState::DISTANT);
	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::DISTANT);

	EXPECT_CALL(mock_sync_linker, unlink(edge));
	//EXPECT_CALL(location_manager, remove(tgt_mock));

	graph.unlink(edge);

	ASSERT_EQ(graph.getEdges().size(), 0);
	delete src_mock;
	delete tgt_mock;
}

TEST_F(DistributedGraphUnlinkTest, distant_src_local_tgt) {
	this->link(LocationState::DISTANT, LocationState::LOCAL);
	ASSERT_EQ(static_cast<MockEdge*>(edge)->_state, LocationState::DISTANT);

	EXPECT_CALL(mock_sync_linker, unlink(edge));
	//EXPECT_CALL(location_manager, remove(src_mock));

	graph.unlink(edge);

	ASSERT_EQ(graph.getEdges().size(), 0);
	delete src_mock;
	delete tgt_mock;
}

/*********************/
/* remove_node_tests */
/*********************/
class DistributedGraphRemoveNodeTest : public DistributedGraphTest {
	protected:
		DistributedId node_id {0, 0};
		MockMutex<int>* mock_mutex = new MockMutex<int>;
		MockNode* node_to_remove = new MockNode(node_id);
		MockNode* edge1_src = new MockNode({0, 1});
		MockEdge* edge1 = new MockEdge({0, 1}, 0);
		MockNode* edge2_tgt = new MockNode({0, 2});
		MockEdge* edge2 = new MockEdge({0, 2}, 0);
		std::vector<fpmas::api::graph::DistributedEdge<int>*> in {edge1};
		std::vector<fpmas::api::graph::DistributedEdge<int>*> out {edge2};

		void SetUp() override {
			DistributedGraphTest::SetUp();

			node_to_remove->setMutex(mock_mutex);

			edge1->src = edge1_src;
			edge1->tgt = node_to_remove;
			edge2->src = node_to_remove;
			edge2->tgt = edge2_tgt;

			graph.insert(node_to_remove);
			graph.insert(edge1_src);
			graph.insert(edge2_tgt);

			graph.insert(edge1);
			graph.insert(edge2);

			ON_CALL(*node_to_remove, getOutgoingEdges())
				.WillByDefault(Return(out));
			EXPECT_CALL(*node_to_remove, getOutgoingEdges())
				.Times(AnyNumber());

			ON_CALL(*node_to_remove, getIncomingEdges())
				.WillByDefault(Return(in));
			EXPECT_CALL(*node_to_remove, getIncomingEdges())
				.Times(AnyNumber());

			EXPECT_CALL(*edge1_src, unlinkOut(edge1));
			EXPECT_CALL(*edge2_tgt, unlinkIn(edge2));
		}
};

TEST_F(DistributedGraphRemoveNodeTest, remove_node) {
	EXPECT_CALL(*node_to_remove, unlinkIn(edge1));
	EXPECT_CALL(*node_to_remove, unlinkOut(edge2));

	EXPECT_CALL(mock_sync_linker, removeNode(node_to_remove));
	EXPECT_CALL(location_manager, remove(node_to_remove));
	Expectation lock = EXPECT_CALL(*mock_mutex, lockShared);
	EXPECT_CALL(*mock_mutex, unlockShared).After(lock);

	graph.removeNode(node_to_remove);

	// Normally called at DistributedGraph::synchronize, depending on the
	// synchronization mode
	graph.erase(node_to_remove);
/*
 *
 *    ASSERT_THAT(graph.getNodes(), Not(Contains(Pair(node_id, node_to_remove)))); 
 *    ASSERT_THAT(graph.getEdges(), Not(IsSupersetOf({
 *                    Pair(DistributedId {0, 1}, edge1),
 *                    Pair(DistributedId {0, 2}, edge2)
 *                    }))); 
 */
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
		typedef typename GraphType::EdgeType EdgeType; // MockDistributedEdge<int, MockMutex>
		typedef typename fpmas::api::graph::PartitionMap PartitionMap;

		PartitionMap partition;
		std::unordered_map<int, std::vector<NodePtrWrapper<int>>> imported_node_mocks;
		std::unordered_map<int, std::vector<EdgePtrWrapper<int>>> imported_edge_mocks;
		
		void SetUp() override {
			DistributedGraphTest::SetUp();
		}

		void distributeTest() {
			EXPECT_CALL(
					(const_cast<MockMpi<NodePtrWrapper<int>>&>(graph.getNodeMpi())),
					migrate
					).WillOnce(Return(imported_node_mocks));
			EXPECT_CALL(
					(const_cast<MockMpi<EdgePtrWrapper<int>>&>(graph.getEdgeMpi())),
					migrate
					).WillOnce(Return(imported_edge_mocks));
			EXPECT_CALL(mock_sync_linker, synchronize).Times(1);
			EXPECT_CALL(mock_data_sync, synchronize).Times(1);
			graph.distribute(partition);
		}

};

/*
 * Import new node test
 */
TEST_F(DistributedGraphImportNodeTest, import_node) {
	imported_node_mocks[1].emplace_back(new MockNode(DistributedId(1, 10), 8, 2.1));

	EXPECT_CALL(location_manager,updateLocations());

	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));

	NodeType* build_mutex_arg;
	EXPECT_CALL(
		const_cast<MockSyncMode<int>&>(graph.getSyncMode()), buildMutex)
		.WillOnce(DoAll(SaveArg<0>(&build_mutex_arg), Return(new MockMutex<int>)));

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
	ASSERT_EQ(build_mutex_arg, node);
	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);
	ASSERT_EQ(node->data(), 8);
	ASSERT_EQ(node->getWeight(), 2.1f);
}

/*
 * Import node with existing ghost test
 */
TEST_F(DistributedGraphImportNodeTest, import_node_with_existing_ghost) {
	EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _));
	EXPECT_CALL(location_manager, setLocal);

	NodeType* build_mutex_arg;
	EXPECT_CALL(const_cast<MockSyncMode<int>&>(graph.getSyncMode()),
		buildMutex).WillOnce(
			DoAll(SaveArg<0>(&build_mutex_arg), Return(new MockMutex<int>)));

	auto node = graph.buildNode(8);
	ON_CALL(*static_cast<MockNode*>(node), state()).WillByDefault(Return(LocationState::DISTANT));

	imported_node_mocks[1].emplace_back(new MockNode(node->getId(), 2, 4.));

	EXPECT_CALL(location_manager, updateLocations());

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

	ASSERT_EQ(build_mutex_arg, node);
	ASSERT_EQ(set_local_arg, node);
	ASSERT_EQ(local_callback_arg, node);
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(node->getId()), 1);
}

/********************/
/* import_edge_tests */
/********************/
/*
 * Test suite for the different cases of edge import when :
 * - the two nodes exist and are LOCAL
 * - the two nodes exist, src is DISTANT, tgt is LOCAL
 * - the two nodes exist, src is LOCAL, tgt is DISTANT
 */
class DistributedGraphImportEdgeTest : public DistributedGraphImportNodeTest {
	protected:
		NodeType* src;
		NodeType* tgt;
		MockNode* mock_src;
		MockNode* mock_tgt;

		EdgeType* imported_edge;
		MockEdge* mock_edge;

	void SetUp() override {
		DistributedGraphImportNodeTest::SetUp();

		EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _)).Times(AnyNumber());
		EXPECT_CALL(location_manager, setLocal).Times(AnyNumber());
		EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(AnyNumber());
		src = graph.buildNode();
		tgt = graph.buildNode();

		mock_src = new MockNode(src->getId());
		mock_tgt = new MockNode(tgt->getId());
		mock_edge = new MockEdge(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);
		mock_edge->src = mock_src;
		mock_edge->tgt = mock_tgt;

		// Mock to serialize and import from proc 1 (not yet in the graph)
		imported_edge_mocks[1].push_back(mock_edge);

		EXPECT_CALL(*static_cast<MockNode*>(src), linkOut(_));
		EXPECT_CALL(*static_cast<MockNode*>(src), unlinkOut(_));
		EXPECT_CALL(*static_cast<MockNode*>(tgt), linkIn(_));
		EXPECT_CALL(*static_cast<MockNode*>(tgt), unlinkIn(_));
	}

	void checkEdgeStructure(DistributedId id = DistributedId(3, 12)) {
		ASSERT_EQ(graph.getEdges().size(), 1);
		ASSERT_EQ(graph.getEdges().count(id), 1);
		imported_edge = graph.getEdge(id);
		ASSERT_EQ(imported_edge->getLayer(), 2);

		ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->src, src);
		ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->tgt, tgt);
	}

};

/*
 * Import an edge already contained in the graph, as a distant edge.
 * Nothing is done, but the operation can occur when migrating edges : a proc
 * can send an edge to an other proc, because it is linked to an exported node,
 * but the edges represented on the target node are not known. In consequence,
 * the imported edge might have already been imported with an other node in a
 * previous iteration.
 */
TEST_F(DistributedGraphImportEdgeTest, import_existing_local_edge) {
	EXPECT_CALL(*static_cast<MockNode*>(src), state)
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_sync_linker, link);

	DistributedId id;
	auto* mocked_existing_edge = graph.link(src, tgt, 2);
	EXPECT_CALL(*static_cast<MockEdge*>(mocked_existing_edge), state)
		.WillRepeatedly(Return(LocationState::DISTANT));
	id = mocked_existing_edge->getId();

	ON_CALL(*mock_edge, getId).WillByDefault(Return(id));

	graph.importEdge(mock_edge);

	checkEdgeStructure(id);
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::DISTANT);
}

/*
 * Import edge when the two nodes are LOCAL
 */
TEST_F(DistributedGraphImportEdgeTest, import_local_edge) {
	EXPECT_CALL(location_manager, updateLocations());

	distributeTest();
	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::LOCAL);
}


/*
 * Import with DISTANT src
 */
TEST_F(DistributedGraphImportEdgeTest, import_edge_with_existing_distant_src) {
	EXPECT_CALL(*static_cast<MockNode*>(src), state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(location_manager, updateLocations());

	distributeTest();
	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::DISTANT);
}

/*
 * Import with DISTANT tgt
 */
TEST_F(DistributedGraphImportEdgeTest, import_edge_with_existing_distant_tgt) {
	EXPECT_CALL(*static_cast<MockNode*>(tgt), state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(location_manager, updateLocations());

	distributeTest();
	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::DISTANT);
}

/*
 * Special edge case that test the import behavior when the same edge is
 * imported twice.
 *
 * This happens notably in the following case :
 * - nodes 0 and 1 are linked and lives in distinct procs
 * - they are both imported to an other proc
 * - in consecuence, because each original proc does not know that the other
 *   node will also be imported to the destination proc, the edge connecting the
 *   two nodes (represented as a ghost edge on both original process) is
 *   imported twice to the destination proc.
 *
 * Obviously, the expected behavior is that no duplicate is created, i.e. the
 * second import has no effect.
 */
TEST_F(DistributedGraphImportEdgeTest, double_import_edge_case) {
	auto mock = new MockEdge(DistributedId(3, 12), 2, 2.6);

	mock->src = new MockNode(src->getId());
	mock->tgt = new MockNode(tgt->getId());

	// The same edge is imported from proc 2
	imported_edge_mocks[2].push_back(mock);
	EXPECT_CALL(location_manager, updateLocations());

	distributeTest();
	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::LOCAL);
}

/**************************************/
/* import_edge_with_missing_node_test */
/**************************************/
/*
 * In the previous test suite, at least one of the two nodes might be DISTANT,
 * but they already exist in the Graph.
 * However, Edges might be imported with a source or target node that has not
 * been added to the Graph yet (has LOCAL or DISTANT node), and in this case
 * the missing Node must be created as a new DISTANT node.
 */
class DistributedGraphImportEdgeWithGhostTest : public DistributedGraphImportNodeTest {
	protected:
		NodeType* local_node;
		MockNode* mock_one;
		MockNode* mock_two;

		EdgeType* importedEdge;

		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(graph.getSyncMode(), buildMutex);
			EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _));
			EXPECT_CALL(location_manager, setLocal);
			local_node = graph.buildNode();

			mock_one = new MockNode(local_node->getId());
			mock_two = new MockNode(DistributedId(8, 16));

			// This node is not contained is the graph, but will be linked.
			ASSERT_EQ(graph.getNodes().count(mock_two->getId()), 0);

			// SetUp should be call once for the created tgt or src
			EXPECT_CALL(graph.getSyncMode(), buildMutex);
		}

		void checkEdgeAndGhostStructure() {
			ASSERT_EQ(graph.getEdges().size(), 1);
			ASSERT_EQ(graph.getEdges().count(DistributedId(3, 12)), 1);
			importedEdge = graph.getEdge(DistributedId(3, 12));
			ASSERT_EQ(importedEdge->getLayer(), 2);
			ASSERT_EQ(importedEdge->getWeight(), 2.6f);
			ASSERT_EQ(static_cast<MockEdge*>(importedEdge)->_state, LocationState::DISTANT);

			EXPECT_EQ(graph.getNodes().size(), 2);
			EXPECT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
			auto ghostNode = graph.getNode(DistributedId(8, 16));
			EXPECT_EQ(ghostNode->getId(), DistributedId(8, 16));
		}
};

/*
 * LOCAL src, missing tgt
 */
TEST_F(DistributedGraphImportEdgeWithGhostTest, import_with_missing_tgt) {
	auto mock = new MockEdge(DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock->src = mock_one;
	mock->tgt = mock_two; // This node is not yet contained in the graph

	// Mock to serialize and import from proc 1 (not yet in the graph)
	imported_edge_mocks[1].push_back(mock);

	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkIn(_)).Times(0);
	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkOut(_));
	EXPECT_CALL(*static_cast<MockNode*>(local_node), unlinkOut);
	EXPECT_CALL(*mock_two, unlinkIn);


	EXPECT_CALL(location_manager, updateLocations());

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
	checkEdgeAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockEdge*>(importedEdge)->src, local_node);
	ASSERT_EQ(static_cast<MockEdge*>(importedEdge)->tgt, distant_node);
}

/*
 * Missing src, LOCAL tgt
 */
TEST_F(DistributedGraphImportEdgeWithGhostTest, import_with_missing_src) {
	auto mock = new MockEdge(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);

	mock->src = mock_two; // This node is not yet contained in the graph
	mock->tgt = mock_one;

	// Mock to serialize and import from proc 1 (not yet in the graph)
	imported_edge_mocks[1].push_back(mock);

	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkOut(_)).Times(0);
	EXPECT_CALL(*static_cast<MockNode*>(local_node), linkIn(_));
	EXPECT_CALL(*static_cast<MockNode*>(local_node), unlinkIn);
	EXPECT_CALL(*mock_two, unlinkOut);

	EXPECT_CALL(location_manager, updateLocations());

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
	checkEdgeAndGhostStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockEdge*>(importedEdge)->tgt, local_node);
	ASSERT_EQ(static_cast<MockEdge*>(importedEdge)->src, distant_node);
}

/***********************/
/* insert_distant_test */
/***********************/

class DistributedGraphInsertDistantTest : public DistributedGraphTest {

	void SetUp() {
		DistributedGraphTest::SetUp();
	}
};

TEST_F(DistributedGraphInsertDistantTest, insert_distant) {
	auto node = new MockDistributedNode<int>;

	EXPECT_CALL(location_manager, setDistant(node));
	MockMutex<int>* built_mutex = new MockMutex<int>;
	EXPECT_CALL(const_cast<MockSyncMode<int>&>(graph.getSyncMode()), buildMutex)
		.WillOnce(Return(built_mutex));
	EXPECT_CALL(*node, setMutex(built_mutex));

	ASSERT_EQ(graph.insertDistant(node), node);
}

TEST_F(DistributedGraphInsertDistantTest, insert_distant_existing_node) {
	auto existing_node = new NiceMock<MockDistributedNode<int>>(DistributedId(2, 4));
	graph.insert(existing_node);

	auto node = new MockDistributedNode<int>(DistributedId(2, 4));

	EXPECT_CALL(location_manager, setDistant(node))
		.Times(0);
	EXPECT_CALL(const_cast<MockSyncMode<int>&>(graph.getSyncMode()), buildMutex)
		.Times(0);

	ASSERT_EQ(graph.insertDistant(node), existing_node);
}

/********************/
/* distribute_tests */
/********************/
class DistributedGraphDistributeTest : public DistributedGraphTest {
	typedef typename fpmas::api::graph::PartitionMap PartitionMap;

	protected:
		std::array<DistributedId, 5> node_ids;
		PartitionMap fake_partition;

		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _)).Times(5);
			EXPECT_CALL(location_manager, setLocal).Times(5);
			EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(5);
			for(int i=0; i < 5;i++) {
				auto node = graph.buildNode();
				node_ids[i] = node->getId();
			}
			fake_partition[node_ids[0]] = 7;
			fake_partition[node_ids[1]] = 1;
			fake_partition[node_ids[2]] = 1;
			fake_partition[node_ids[3]] = 0;
			fake_partition[node_ids[4]] = 0;
		}
};

/*
 * Check the correct call order of the synchronization steps of the
 * distribute() function.
 * 1. Synchronize links (eventually migrates link/unlink operations)
 * 2. Migrate node / edges
 * 3. Synchronize Node locations
 * 4. Synchronize Node data
 */
TEST_F(DistributedGraphTest, distribute_calls) {
	Sequence s;
	EXPECT_CALL(
			mock_sync_linker,
			synchronize()
			)
		.InSequence(s);

	EXPECT_CALL(
			comm, allToAll)
		.Times(AnyNumber())
		.InSequence(s);
	EXPECT_CALL(location_manager, updateLocations())
		.InSequence(s);
	EXPECT_CALL(
			mock_data_sync,
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
			mock_sync_linker,
			synchronize())
		.InSequence(s);

	EXPECT_CALL(
			mock_data_sync,
			synchronize())
		.InSequence(s);

	EXPECT_CALL(location_manager, getDistantNodes).WillOnce(ReturnRefOfCopy(NodeMap()));
	graph.synchronize();
}

/*
 * Balance test.
 * Checks that load balancing is called and migration is performed.
 */
TEST_F(DistributedGraphDistributeTest, balance) {
	MockLoadBalancing<int> load_balancing;
	fpmas::api::graph::NodeMap<int> node_map;
	for(auto node : graph.getNodes())
		node_map.insert(node);

	// Should call LoadBalancing on all nodes, without fixed nodes
	{
		InSequence s;
		EXPECT_CALL(mock_sync_linker, synchronize);
		EXPECT_CALL(
				load_balancing,
				balance(node_map))
			.WillOnce(Return(fake_partition));
		EXPECT_CALL(mock_data_sync, synchronize);
	}
	// Migration of nodes + edges
	EXPECT_CALL(graph.getNodeMpi(), migrate);
	EXPECT_CALL(graph.getEdgeMpi(), migrate);

	EXPECT_CALL(location_manager, getLocalNodes).WillRepeatedly(ReturnRef(node_map));
	EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());
	EXPECT_CALL(location_manager, remove).Times(4);
	EXPECT_CALL(location_manager, updateLocations());

	// Actual call
	graph.balance(load_balancing);
}

/*
 * Node distribution test (no edges)
 */
TEST_F(DistributedGraphDistributeTest, distribute_without_link) {
	auto export_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					graph.getNode(node_ids[3]),
					graph.getNode(node_ids[4])
					)),
			Pair(1, UnorderedElementsAre(
					graph.getNode(node_ids[1]),
					graph.getNode(node_ids[2])
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

	// No edge import
	EXPECT_CALL(
			const_cast<MockMpi<EdgePtrWrapper<int>>&>(graph.getEdgeMpi()), migrate(IsEmpty()))
		.WillOnce(Return(std::unordered_map<int, std::vector<EdgePtrWrapper<int>>>()));;

	EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());
	EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[1])));
	EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[2])));
	EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[3])));
	EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[4])));

	std::array<NodeType*, 2> set_local_args;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_args[0]))
		.WillOnce(SaveArg<0>(&set_local_args[1]));
	EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(2);
	
	EXPECT_CALL(location_manager, updateLocations());

	EXPECT_CALL(mock_sync_linker, synchronize);
	EXPECT_CALL(mock_data_sync, synchronize);
	graph.distribute(fake_partition);
	ASSERT_EQ(graph.getNodes().size(), 3);
	ASSERT_EQ(graph.getNodes().count(node_ids[0]), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(2, 5)), 1);
	ASSERT_EQ(graph.getNodes().count(DistributedId(4, 3)), 1);

	EXPECT_THAT(set_local_args, UnorderedElementsAre(
			graph.getNode(DistributedId(2, 5)),
			graph.getNode(DistributedId(4, 3))
			));
}

/****************************/
/* distribute_test_with_edge */
/****************************/
class DistributedGraphDistributeWithLinkTest : public DistributedGraphDistributeTest {
	protected:
		typedef std::vector<fpmas::api::graph::DistributedEdge<int>*> EdgeList;

		EdgeType* edge1;
		EdgeType* edge2;

		EdgeList node2_in;
		EdgeList node3_out;

		MockCallback<NodeType*>* distant_callback = new MockCallback<NodeType*>;
		std::array<NodeType*, 2> imported_edge_distant_callback_args;

		void SetUp() override {
			DistributedGraphDistributeTest::SetUp();

			// No lock to manage, all links are local
			MockMutex<int> mock_mutex;
			EXPECT_CALL(mock_mutex, lockShared).Times(AnyNumber());
			EXPECT_CALL(mock_mutex, unlockShared).Times(AnyNumber());
			EXPECT_CALL(mock_sync_linker, link).Times(2);

			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[0])), mutex())
				.WillRepeatedly(Return(&mock_mutex));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[2])), mutex())
				.WillRepeatedly(Return(&mock_mutex));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[3])), mutex())
				.WillRepeatedly(Return(&mock_mutex));

			edge1 = graph.link(graph.getNode(node_ids[0]), graph.getNode(node_ids[2]), 0);
			node2_in.push_back(edge1);
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[2])), getIncomingEdges())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&node2_in));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[2])), unlinkIn)
				.WillOnce(InvokeWithoutArgs(&node2_in, &EdgeList::clear));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[0])), unlinkOut);


			edge2 = graph.link(graph.getNode(node_ids[3]), graph.getNode(node_ids[0]), 0);
			node3_out.push_back(edge2);
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[3])), getOutgoingEdges())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&node3_out));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[3])), unlinkOut)
				.WillOnce(InvokeWithoutArgs(&node3_out, &EdgeList::clear));
			EXPECT_CALL(*static_cast<MockNode*>(graph.getNode(node_ids[0])), unlinkIn);

			// Other uninteresting setDistant calls might occur on unconnected
			// nodes that will be cleared
			EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());

			graph.addCallOnSetDistant(distant_callback);

			// Those two expectation are matched only when the next 4
			// expectations are not matched.
			EXPECT_CALL(*distant_callback, call)
				.WillOnce(SaveArg<0>(&imported_edge_distant_callback_args[0]))
				.WillOnce(SaveArg<0>(&imported_edge_distant_callback_args[1]));

			// Exported node callback matchers
			EXPECT_CALL(*distant_callback, call(graph.getNode(node_ids[2])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(node_ids[3])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(node_ids[1])));
			EXPECT_CALL(*distant_callback, call(graph.getNode(node_ids[4])));

			// Set linked nodes as distant nodes
			EXPECT_CALL(location_manager, setDistant(graph.getNode(node_ids[2])));
			EXPECT_CALL(location_manager, setDistant(graph.getNode(node_ids[3])));

			EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[1])));
			EXPECT_CALL(location_manager, remove(graph.getNode(node_ids[4])));
		}

		void TearDown() override {
		}
};

/*
 * Distribute nodes + edges
 */
TEST_F(DistributedGraphDistributeWithLinkTest, distribute_with_link_test) {
	auto export_node_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					graph.getNode(node_ids[3]),
					graph.getNode(node_ids[4])
					)),
			Pair(1, UnorderedElementsAre(
					graph.getNode(node_ids[1]),
					graph.getNode(node_ids[2])
					))
			);
	// No node import
	EXPECT_CALL(
		const_cast<MockMpi<NodePtrWrapper<int>>&>(graph.getNodeMpi()), migrate(export_node_map_matcher));

	auto export_edge_map_matcher = UnorderedElementsAre(
		Pair(0, ElementsAre(edge2)),
		Pair(1, ElementsAre(edge1))
	);

	auto edge = new MockEdge(DistributedId(4, 6), 2);
	std::unordered_map<int, std::vector<EdgePtrWrapper<int>>> edge_import {
		{3, {edge}}
	};
	auto mock_src = new MockNode(DistributedId(4, 8));
	auto mock_tgt = new MockNode(DistributedId(3, 2));
	edge->src = mock_src;
	edge->tgt = mock_tgt;
	EXPECT_CALL(*mock_src, unlinkOut);
	EXPECT_CALL(*mock_tgt, unlinkIn);

	EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(2);

	// Mock edge import
	EXPECT_CALL(
		const_cast<MockMpi<EdgePtrWrapper<int>>&>(graph.getEdgeMpi()),
		migrate(export_edge_map_matcher))
		.WillOnce(Return(edge_import));

	EXPECT_CALL(location_manager, updateLocations());
	EXPECT_CALL(mock_sync_linker, synchronize);
	EXPECT_CALL(mock_data_sync, synchronize);
	graph.distribute(fake_partition);

	ASSERT_THAT(imported_edge_distant_callback_args, UnorderedElementsAre(mock_src, mock_tgt));
	ASSERT_EQ(graph.getEdges().size(), 3);
	ASSERT_EQ(graph.getEdges().count(DistributedId(4, 6)), 1);
	ASSERT_EQ(graph.getEdges().count(edge1->getId()), 1);
	ASSERT_EQ(graph.getEdges().count(edge2->getId()), 1);
}

class DistributedGraphMoveTest : public Test {
	protected:
		MockMpiCommunicator<> comm;
		template<typename T>
			class NiceMockSyncMode : public NiceMock<MockSyncMode<T>> {
				public:
					NiceMock<MockSyncLinker<int>> mock_sync_linker;

					NiceMockSyncMode() {
						ON_CALL(*this, getSyncLinker())
							.WillByDefault(ReturnRef(mock_sync_linker));
					}
					NiceMockSyncMode(
							fpmas::api::graph::DistributedGraph<T>&,
							fpmas::api::communication::MpiCommunicator&)
						: NiceMockSyncMode() {
						}

					NiceMockSyncMode(NiceMockSyncMode&&)
						: NiceMockSyncMode() {}
					NiceMockSyncMode& operator=(NiceMockSyncMode&&) {return *this;}
			};
		typedef 
		fpmas::graph::detail::DistributedGraph<
			int, NiceMockSyncMode,
			fpmas::graph::DistributedNode, fpmas::graph::DistributedEdge,
			MockMpi, fpmas::graph::LocationManager> TestGraph;
		TestGraph graph {comm};
		fpmas::random::mt19937 rd;

		void build_graph(
				fpmas::api::graph::DistributedGraph<int>& graph,
				std::size_t num_callbacks, std::size_t num_nodes, std::size_t num_edges) {
			for(std::size_t i = 0; i < num_callbacks; i++) {
				graph.addCallOnInsertNode(new NiceMock<MockCallback<fpmas::api::graph::DistributedNode<int>*>>);
				graph.addCallOnEraseNode(new NiceMock<MockCallback<fpmas::api::graph::DistributedNode<int>*>>);
				graph.addCallOnSetLocal(new NiceMock<MockCallback<fpmas::api::graph::DistributedNode<int>*>>);
				graph.addCallOnSetDistant(new NiceMock<MockCallback<fpmas::api::graph::DistributedNode<int>*>>);
				graph.addCallOnInsertEdge(new NiceMock<MockCallback<fpmas::api::graph::DistributedEdge<int>*>>);
				graph.addCallOnEraseEdge(new NiceMock<MockCallback<fpmas::api::graph::DistributedEdge<int>*>>);
			}
			std::vector<fpmas::api::graph::DistributedNode<int>*> nodes;
			for(std::size_t i = 0; i < num_nodes; i++)
				nodes.push_back(graph.buildNode(0));
			fpmas::random::UniformIntDistribution<std::size_t> rd_id(0, nodes.size()-1);
			for(std::size_t i = 0; i < num_edges; i++)
				graph.link(nodes[rd_id(rd)], nodes[rd_id(rd)], 0);
		}

		void SetUp() override {

			build_graph(graph, 2, 10, 20);
		}
};

TEST_F(DistributedGraphMoveTest, move_constructor) {
	auto insert_node = this->graph.onInsertNodeCallbacks();
	auto insert_edge = this->graph.onInsertEdgeCallbacks();
	auto erase_node = this->graph.onEraseNodeCallbacks();
	auto erase_edge = this->graph.onEraseEdgeCallbacks();
	auto set_local = this->graph.onSetLocalCallbacks();
	auto set_distant = this->graph.onSetDistantCallbacks();
	auto nodes = this->graph.getNodes();
	auto edges = this->graph.getEdges();
	auto node_id = this->graph.currentNodeId();
	auto edge_id = this->graph.currentEdgeId();
	auto locations = this->graph.getLocationManager().getCurrentLocations();
	auto local_nodes = this->graph.getLocationManager().getLocalNodes();
	auto distant_nodes = this->graph.getLocationManager().getDistantNodes();

	TestGraph new_graph(std::move(graph));

	ASSERT_THAT(
			new_graph.getLocationManager().getCurrentLocations(),
			UnorderedElementsAreArray(locations));
	ASSERT_THAT(
			new_graph.getLocationManager().getLocalNodes(),
			UnorderedElementsAreArray(local_nodes));
	ASSERT_THAT(
			new_graph.getLocationManager().getDistantNodes(),
			UnorderedElementsAreArray(distant_nodes));

	ASSERT_THAT(new_graph.currentNodeId(), node_id);
	ASSERT_THAT(new_graph.currentEdgeId(), edge_id);

	ASSERT_THAT(this->graph.getNodes(), IsEmpty());
	ASSERT_THAT(this->graph.getEdges(), IsEmpty());
	ASSERT_THAT(new_graph.getNodes(), UnorderedElementsAreArray(nodes));
	ASSERT_THAT(new_graph.getEdges(), UnorderedElementsAreArray(edges));

	ASSERT_THAT(this->graph.onInsertNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onInsertEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onSetLocalCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onSetDistantCallbacks(), IsEmpty());

	ASSERT_THAT(new_graph.onInsertNodeCallbacks(), UnorderedElementsAreArray(insert_node));
	ASSERT_THAT(new_graph.onEraseNodeCallbacks(), UnorderedElementsAreArray(erase_node));
	ASSERT_THAT(new_graph.onInsertEdgeCallbacks(), UnorderedElementsAreArray(insert_edge));
	ASSERT_THAT(new_graph.onEraseEdgeCallbacks(), UnorderedElementsAreArray(erase_edge));
	ASSERT_THAT(new_graph.onSetLocalCallbacks(), UnorderedElementsAreArray(set_local));
	ASSERT_THAT(new_graph.onSetDistantCallbacks(), UnorderedElementsAreArray(set_distant));
}

TEST_F(DistributedGraphMoveTest, move_assignment) {
	auto insert_node = this->graph.onInsertNodeCallbacks();
	auto insert_edge = this->graph.onInsertEdgeCallbacks();
	auto erase_node = this->graph.onEraseNodeCallbacks();
	auto erase_edge = this->graph.onEraseEdgeCallbacks();
	auto set_local = this->graph.onSetLocalCallbacks();
	auto set_distant = this->graph.onSetDistantCallbacks();
	auto nodes = this->graph.getNodes();
	auto edges = this->graph.getEdges();
	auto node_id = this->graph.currentNodeId();
	auto edge_id = this->graph.currentEdgeId();
	auto locations = this->graph.getLocationManager().getCurrentLocations();
	auto local_nodes = this->graph.getLocationManager().getLocalNodes();
	auto distant_nodes = this->graph.getLocationManager().getDistantNodes();

	MockMpiCommunicator<> other_comm;
	TestGraph new_graph(other_comm);
	build_graph(new_graph, 3, 45, 24);
	new_graph = std::move(this->graph);

	ASSERT_THAT(new_graph.getMpiCommunicator(), Ref(comm));

	ASSERT_THAT(
			new_graph.getLocationManager().getCurrentLocations(),
			UnorderedElementsAreArray(locations));
	ASSERT_THAT(
			new_graph.getLocationManager().getLocalNodes(),
			UnorderedElementsAreArray(local_nodes));
	ASSERT_THAT(
			new_graph.getLocationManager().getDistantNodes(),
			UnorderedElementsAreArray(distant_nodes));

	ASSERT_THAT(new_graph.currentNodeId(), node_id);
	ASSERT_THAT(new_graph.currentEdgeId(), edge_id);

	ASSERT_THAT(this->graph.getNodes(), IsEmpty());
	ASSERT_THAT(this->graph.getEdges(), IsEmpty());
	ASSERT_THAT(new_graph.getNodes(), UnorderedElementsAreArray(nodes));
	ASSERT_THAT(new_graph.getEdges(), UnorderedElementsAreArray(edges));

	ASSERT_THAT(this->graph.onInsertNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onInsertEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onSetLocalCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onSetDistantCallbacks(), IsEmpty());

	ASSERT_THAT(new_graph.onInsertNodeCallbacks(), UnorderedElementsAreArray(insert_node));
	ASSERT_THAT(new_graph.onEraseNodeCallbacks(), UnorderedElementsAreArray(erase_node));
	ASSERT_THAT(new_graph.onInsertEdgeCallbacks(), UnorderedElementsAreArray(insert_edge));
	ASSERT_THAT(new_graph.onEraseEdgeCallbacks(), UnorderedElementsAreArray(erase_edge));
	ASSERT_THAT(new_graph.onSetLocalCallbacks(), UnorderedElementsAreArray(set_local));
	ASSERT_THAT(new_graph.onSetDistantCallbacks(), UnorderedElementsAreArray(set_distant));
}
