/*********/
/* Index */
/*********/
/*
 * # local_build_node
 * # distributed_calls
 * # link_tests
 * # unlink_tests
 * # switch_layer_tests
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

#include "communication/mock_communication.h"
#include "fpmas/api/graph/graph.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/random/random.h"
#include "graph/mock_distributed_node.h"
#include "graph/mock_distributed_edge.h"
#include "graph/mock_location_manager.h"
#include "synchro/mock_mutex.h"
#include "synchro/mock_sync_mode.h"
#include "graph/mock_load_balancing.h"
#include "utils/mock_callback.h"

using namespace testing;

using fpmas::graph::detail::DistributedGraph;
using fpmas::graph::NodePtrWrapper;
using fpmas::graph::EdgePtrWrapper;
using fpmas::api::communication::DataPack;
using fpmas::api::graph::SetLocalNodeEvent;
using fpmas::api::graph::SetDistantNodeEvent;


namespace fpmas { namespace api { namespace graph {
	template<typename T>
		bool operator==(
				const SetLocalNodeEvent<T>& event_a,
				const SetLocalNodeEvent<T>& event_b
				) {
			return
				event_a.node == event_b.node &&
				event_a.context == event_b.context;
		}

	template<typename T>
		bool operator==(
				const SetDistantNodeEvent<T>& event_a,
				const SetDistantNodeEvent<T>& event_b
				) {
			return
				event_a.node == event_b.node &&
				event_a.context == event_b.context;
		}
}}}

MATCHER_P(SetLocalNodeEventContextIs, context, "") {
	return ExplainMatchResult(Field(
				&SetLocalNodeEvent<int>::context,
				context
				), arg, result_listener
			);
}

MATCHER_P(SetDistantNodeEventContextIs, context, "") {
	return ExplainMatchResult(Field(
				&SetDistantNodeEvent<int>::context,
				context
				), arg, result_listener
			);
}

ACTION_P(SaveNode, node_pointer) {*node_pointer = arg0.node;}

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
		MockDataSync<int> mock_data_sync;

	
		void SetUp() override {
			ON_CALL(graph.getSyncMode(), getSyncLinker)
				.WillByDefault(ReturnRef(mock_sync_linker));
			ON_CALL(graph.getSyncMode(), getDataSync)
				.WillByDefault(ReturnRef(mock_data_sync));
		}

};

/********************/
/* local_build_node */
/********************/
TEST_F(DistributedGraphTest, build_node) {
	auto local_callback
		= new MockEventCallback<SetLocalNodeEvent<int>>;
	graph.addCallOnSetLocal(local_callback);

	DistributedId add_managed_node_arg;
	EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), CURRENT_RANK))
		.WillOnce(SaveArg<0>(&add_managed_node_arg));
	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call(
				SetLocalNodeEventContextIs(SetLocalNodeEvent<int>::BUILD_LOCAL)
				))
		.WillOnce(SaveNode(&local_callback_arg));

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

/********************/
/* distribute_calls */
/********************/

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

TEST_F(DistributedGraphTest, partial_synchronize_calls) {
	std::array<fpmas::api::graph::DistributedNode<int>*, 3> fake_nodes {
		new MockNode({0, 0}), new MockNode({0, 1}), new MockNode({0, 2})
	};
	EXPECT_CALL(location_manager, setDistant).Times(2);
	graph.insertDistant(fake_nodes[0]);
	graph.insert(fake_nodes[1]); // DISTANT, but not unsynced
	graph.insertDistant(fake_nodes[2]);

	Sequence s;
	EXPECT_CALL(
			mock_sync_linker,
			synchronize())
		.InSequence(s);

	EXPECT_CALL(
			mock_data_sync,
			synchronize(UnorderedElementsAre(fake_nodes[0], fake_nodes[1])))
		.InSequence(s);

	EXPECT_CALL(location_manager, getDistantNodes).WillOnce(ReturnRefOfCopy(NodeMap()));
	graph.synchronize({fake_nodes[0], fake_nodes[1]});

	ASSERT_THAT(graph.getUnsyncNodes(), ElementsAre(fake_nodes[2]));
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
/**********************/
/* switch_layer_tests */
/**********************/
class DistributedGraphSwitchLayerTest : public Test {
	protected:
		static const int CURRENT_RANK = 7;
		MockMpiCommunicator<CURRENT_RANK, 10> comm;
		template<typename T>
			using MockSyncMode = NiceMock<MockSyncMode<T>>;
		template<typename T>
			using MockLocationManager = NiceMock<MockLocationManager<T>>;
		DistributedGraph<
			int,
			MockSyncMode,
			fpmas::graph::DistributedNode,
			fpmas::graph::DistributedEdge,
			MockMpi,
			MockLocationManager
			> graph {comm};

		fpmas::api::graph::DistributedNode<int>* src;
		fpmas::api::graph::DistributedNode<int>* tgt;
		fpmas::api::graph::DistributedEdge<int>* edge;

		MockSyncLinker<int> mock_sync_linker;
		MockDataSync<int> mock_data_sync;

	public:
		void SetUp() override {
			EXPECT_CALL(mock_sync_linker, link).Times(AnyNumber());

			ON_CALL(graph.getSyncMode(), getSyncLinker)
				.WillByDefault(ReturnRef(mock_sync_linker));
			ON_CALL(graph.getSyncMode(), getDataSync)
				.WillByDefault(ReturnRef(mock_data_sync));

			src = graph.buildNode(0);
			tgt = graph.buildNode(1);
			edge = graph.link(src, tgt, 0);
		}
};

TEST_F(DistributedGraphSwitchLayerTest, switch_layer) {
	graph.switchLayer(edge, 12);

	ASSERT_EQ(edge->getLayer(), 12);
	ASSERT_EQ(edge->getSourceNode(), src);
	ASSERT_EQ(edge->getTargetNode(), tgt);

	ASSERT_THAT(src->getOutgoingEdges(), SizeIs(1));
	ASSERT_THAT(tgt->getIncomingEdges(), SizeIs(1));

	ASSERT_THAT(src->getOutgoingEdges(0), IsEmpty());
	ASSERT_THAT(tgt->getIncomingEdges(0), IsEmpty());

	ASSERT_THAT(src->getOutgoingEdges(12), ElementsAre(edge));
	ASSERT_THAT(tgt->getIncomingEdges(12), ElementsAre(edge));
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

		void setUpNodeToRemove(fpmas::graph::LocationState state) {

			edge1->src = edge1_src;
			edge1->tgt = node_to_remove;
			edge2->src = node_to_remove;
			edge2->tgt = edge2_tgt;

			switch(state) {
				case fpmas::api::graph::LOCAL:
					node_to_remove->setMutex(mock_mutex);
					graph.insert(node_to_remove);
					break;
				case fpmas::api::graph::DISTANT:
					EXPECT_CALL(location_manager, setDistant);
					graph.insertDistant(node_to_remove);
			};

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
		void SetUp() override {
			DistributedGraphTest::SetUp();
		}
};

TEST_F(DistributedGraphRemoveNodeTest, remove_node) {
	setUpNodeToRemove(fpmas::api::graph::LOCAL);

	EXPECT_CALL(*node_to_remove, unlinkIn(edge1));
	EXPECT_CALL(*node_to_remove, unlinkOut(edge2));

	EXPECT_CALL(mock_sync_linker, removeNode(node_to_remove));
	EXPECT_CALL(location_manager, remove(node_to_remove));

	graph.removeNode(node_to_remove);

	// Normally called at DistributedGraph::synchronize, depending on the
	// synchronization mode
	graph.erase(node_to_remove);
}

TEST_F(DistributedGraphRemoveNodeTest, erase_unsync_node) {
	setUpNodeToRemove(fpmas::api::graph::DISTANT);
	auto other_node = new MockNode({0, 3});
	EXPECT_CALL(location_manager, setDistant);
	graph.insertDistant(other_node);


	// Minimalist expectations to prevent gtest warnings, not interesting here
	// because already tested in the previous test
	EXPECT_CALL(mock_sync_linker, removeNode(node_to_remove));
	EXPECT_CALL(location_manager, remove(node_to_remove));

	graph.removeNode(node_to_remove);

	// Normally called at DistributedGraph::synchronize, depending on the
	// synchronization mode
	graph.erase(node_to_remove);

	ASSERT_THAT(graph.getUnsyncNodes(), ElementsAre(other_node));

	// The insertDistant() method builds a mutex, so mock_mutex is not used in
	// this case.
	delete mock_mutex;
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
		
		void SetUp() override {
			DistributedGraphTest::SetUp();
		}

};

/*
 * Import new node test
 */
TEST_F(DistributedGraphImportNodeTest, import_node) {
	MockNode* mock_node = new MockNode(DistributedId(1, 10), 8, 2.1);

	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));

	NodeType* build_mutex_arg;
	EXPECT_CALL(
		const_cast<MockSyncMode<int>&>(graph.getSyncMode()), buildMutex)
		.WillOnce(DoAll(SaveArg<0>(&build_mutex_arg), Return(new MockMutex<int>)));

	// Callback call test
	auto local_callback
		= new MockEventCallback<SetLocalNodeEvent<int>>;
	graph.addCallOnSetLocal(local_callback);
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call(
				SetLocalNodeEventContextIs(SetLocalNodeEvent<int>::IMPORT_NEW_LOCAL)
				))
		.WillOnce(SaveNode(&local_callback_arg));

	graph.importNode(mock_node);

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
TEST_F(DistributedGraphImportNodeTest, import_node_with_existing_distant) {
	EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _));
	EXPECT_CALL(location_manager, setLocal);

	NodeType* build_mutex_arg;
	EXPECT_CALL(const_cast<MockSyncMode<int>&>(graph.getSyncMode()),
		buildMutex).WillOnce(
			DoAll(SaveArg<0>(&build_mutex_arg), Return(new MockMutex<int>)));

	auto node = graph.buildNode(8);
	ON_CALL(*static_cast<MockNode*>(node), state())
		.WillByDefault(Return(LocationState::DISTANT));

	MockNode* mock_node = new MockNode(node->getId(), 2, 4.);

	NodeType* set_local_arg;
	EXPECT_CALL(location_manager, setLocal(_))
		.WillOnce(SaveArg<0>(&set_local_arg));

	// Callback call test
	auto local_callback
		= new MockEventCallback<SetLocalNodeEvent<int>>;
	graph.addCallOnSetLocal(local_callback);
	NodeType* local_callback_arg;
	EXPECT_CALL(*local_callback, call(
				SetLocalNodeEventContextIs(SetLocalNodeEvent<int>::IMPORT_EXISTING_LOCAL)
				))
		.WillOnce(SaveNode(&local_callback_arg));

	graph.importNode(mock_node);

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
		MockTemporaryNode<int>* mock_temp_src;
		NodeType* src;

		MockTemporaryNode<int>* mock_temp_tgt;
		NodeType* tgt;

		MockEdge* mock_edge;

		EdgeType* imported_edge;

	void SetUp() override {
		DistributedGraphImportNodeTest::SetUp();

		EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _)).Times(AnyNumber());
		EXPECT_CALL(location_manager, setLocal).Times(AnyNumber());
		EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(AnyNumber());
		src = graph.buildNode();
		tgt = graph.buildNode();

		mock_edge = new MockEdge(
					DistributedId(3, 12),
					2, // layer
					2.6 // weight
					);
		mock_temp_src = new NiceMock<MockTemporaryNode<int>>;
		ON_CALL(*mock_temp_src, getId)
			.WillByDefault(Return(src->getId()));
		mock_edge->temp_src
			= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_src);

		mock_temp_tgt = new NiceMock<MockTemporaryNode<int>>;
		ON_CALL(*mock_temp_tgt, getId)
			.WillByDefault(Return(tgt->getId()));
		mock_edge->temp_tgt
			= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_tgt);

		// In this test suite, the two nodes are not used by the importEdge
		// operation
		EXPECT_CALL(*mock_temp_src, build)
			.Times(0);
		EXPECT_CALL(*mock_temp_src, destructor);

		EXPECT_CALL(*mock_temp_tgt, build)
			.Times(0);
		EXPECT_CALL(*mock_temp_tgt, destructor);
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

	graph.importEdge(mock_edge);

	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::LOCAL);
}


/*
 * Import with DISTANT src
 */
TEST_F(DistributedGraphImportEdgeTest, import_edge_with_existing_distant_src) {
	EXPECT_CALL(*static_cast<MockNode*>(src), state)
		.WillRepeatedly(Return(LocationState::DISTANT));

	graph.importEdge(mock_edge);
	checkEdgeStructure();
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::DISTANT);
}

/*
 * Import with DISTANT tgt
 */
TEST_F(DistributedGraphImportEdgeTest, import_edge_with_existing_distant_tgt) {
	EXPECT_CALL(*static_cast<MockNode*>(tgt), state)
		.WillRepeatedly(Return(LocationState::DISTANT));

	graph.importEdge(mock_edge);
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

	MockTemporaryNode<int>* mock_temp_src = new NiceMock<MockTemporaryNode<int>>;
	ON_CALL(*mock_temp_src, getId)
		.WillByDefault(Return(src->getId()));
	EXPECT_CALL(*mock_temp_src, destructor);

	MockTemporaryNode<int>* mock_temp_tgt = new NiceMock<MockTemporaryNode<int>>;
	ON_CALL(*mock_temp_tgt, getId)
		.WillByDefault(Return(tgt->getId()));
	EXPECT_CALL(*mock_temp_tgt, destructor);

	// The second edge's nodes is not used
	EXPECT_CALL(*mock_temp_src, build)
		.Times(0);
	EXPECT_CALL(*mock_temp_tgt, build)
		.Times(0);

	mock->temp_src
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_src);
	mock->temp_tgt
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_tgt);

	// Imports the same edge twice (or at least an edge with the same id);
	graph.importEdge(mock_edge);
	graph.importEdge(mock);

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
		MockTemporaryNode<int>* mock_temp_one;
		MockNode* mock_one;
		MockTemporaryNode<int>* mock_temp_two;
		MockNode* mock_two;

		EdgeType* imported_edge;

		void SetUp() override {
			DistributedGraphTest::SetUp();

			EXPECT_CALL(graph.getSyncMode(), buildMutex);
			EXPECT_CALL(location_manager, addManagedNode(An<fpmas::graph::DistributedId>(), _));
			EXPECT_CALL(location_manager, setLocal);
			local_node = graph.buildNode();

			// mock_one = local_node
			mock_temp_one = new NiceMock<MockTemporaryNode<int>>;
			mock_one = new MockNode(local_node->getId());
			ON_CALL(*mock_temp_one, getId)
				.WillByDefault(Return(local_node->getId()));
			// Arbitrary location, different from current rank
			ON_CALL(*mock_temp_one, getLocation)
				.WillByDefault(Return(CURRENT_RANK-1));

			mock_temp_two = new NiceMock<MockTemporaryNode<int>>;
			mock_two = new MockNode(DistributedId(8, 16));
			ON_CALL(*mock_temp_two, getId)
				.WillByDefault(Return(DistributedId(8, 16)));
			// Arbitrary location, different from current rank
			ON_CALL(*mock_temp_two, getLocation)
				.WillByDefault(Return(CURRENT_RANK+1));

			// This node is not contained is the graph, but will be linked.
			ASSERT_EQ(graph.getNodes().count(mock_two->getId()), 0);

			// This node is unused in the importEdge operation
			EXPECT_CALL(*mock_temp_one, build)
				.Times(0);
			EXPECT_CALL(*mock_temp_one, destructor)
				.WillOnce([this] () {delete mock_one;});
			// This node is inserted in the graph, that takes ownership
			EXPECT_CALL(*mock_temp_two, build)
				.WillOnce(Return(mock_two));
			EXPECT_CALL(*mock_temp_two, destructor);

			// SetUp should be call once for the created tgt or src
			EXPECT_CALL(graph.getSyncMode(), buildMutex);
		}

		void checkEdgeAndDistantNodesStructure() {
			ASSERT_EQ(graph.getEdges().size(), 1);
			ASSERT_EQ(graph.getEdges().count(DistributedId(3, 12)), 1);
			imported_edge = graph.getEdge(DistributedId(3, 12));
			ASSERT_EQ(imported_edge->getLayer(), 2);
			ASSERT_EQ(imported_edge->getWeight(), 2.6f);
			ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->_state, LocationState::DISTANT);

			EXPECT_EQ(graph.getNodes().size(), 2);
			EXPECT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
			auto distant_node = graph.getNode(DistributedId(8, 16));
			EXPECT_EQ(distant_node->getId(), DistributedId(8, 16));
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

	mock->temp_src
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_one);
	mock->temp_tgt // This node is not yet contained in the graph
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_two);

	NodeType* set_distant_arg;
	EXPECT_CALL(location_manager, setDistant(_))
		.WillOnce(SaveArg<0>(&set_distant_arg));

	// Distant call back test
	auto* distant_callback
		= new MockEventCallback<SetDistantNodeEvent<int>>;
	graph.addCallOnSetDistant(distant_callback);
	NodeType* distant_callback_arg;
	EXPECT_CALL(*distant_callback, call(
				SetDistantNodeEventContextIs(SetDistantNodeEvent<int>::IMPORT_NEW_DISTANT)
				))
		.WillOnce(SaveNode(&distant_callback_arg));

	graph.importEdge(mock);
	checkEdgeAndDistantNodesStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->src, local_node);
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->tgt, distant_node);
	ASSERT_THAT(graph.getUnsyncNodes(), ElementsAre(Property(
					"getId",
					&fpmas::api::graph::DistributedNode<int>::getId,
					DistributedId(8, 16)
					))
			);
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

	mock->temp_src // This node is not yet contained in the graph
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_two);
	mock->temp_tgt
		= std::unique_ptr<fpmas::api::graph::TemporaryNode<int>>(mock_temp_one);

	NodeType* set_distant_arg;
	EXPECT_CALL(location_manager, setDistant(_))
		.WillOnce(SaveArg<0>(&set_distant_arg));
	// Distant call back test
	auto* distant_callback
		= new MockEventCallback<SetDistantNodeEvent<int>>;
	graph.addCallOnSetDistant(distant_callback);
	NodeType* distant_callback_arg;
	EXPECT_CALL(*distant_callback, call(
				SetDistantNodeEventContextIs(SetDistantNodeEvent<int>::IMPORT_NEW_DISTANT)
				))
		.WillOnce(SaveNode(&distant_callback_arg));

	graph.importEdge(mock);
	checkEdgeAndDistantNodesStructure();

	ASSERT_EQ(graph.getNodes().count(DistributedId(8, 16)), 1);
	auto distant_node = graph.getNode(DistributedId(8, 16));
	ASSERT_EQ(set_distant_arg, distant_node);
	ASSERT_EQ(distant_callback_arg, distant_node);

	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->tgt, local_node);
	ASSERT_EQ(static_cast<MockEdge*>(imported_edge)->src, distant_node);
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

	// Distant call back test
	auto* distant_callback
		= new MockEventCallback<SetDistantNodeEvent<int>>;
	graph.addCallOnSetDistant(distant_callback);
	EXPECT_CALL(*distant_callback, call(SetDistantNodeEvent<int>(
					node,
					SetDistantNodeEvent<int>::IMPORT_NEW_DISTANT
					)));

	ASSERT_EQ(graph.insertDistant(node), node);

	ASSERT_THAT(graph.getUnsyncNodes(), Contains(node));
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
	ASSERT_THAT(graph.getUnsyncNodes(), Not(Contains(node)));
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
				balance(node_map, fpmas::api::graph::PARTITION))
			.WillOnce(Return(fake_partition));
		EXPECT_CALL(mock_data_sync, synchronize(An<std::unordered_set<fpmas::api::graph::DistributedNode<int>*>>()));
	}
	// Migration of nodes + edges
	EXPECT_CALL(comm, allToAll).Times(2);

	EXPECT_CALL(location_manager, getLocalNodes).WillRepeatedly(ReturnRef(node_map));
	EXPECT_CALL(location_manager, setDistant).Times(AnyNumber());
	EXPECT_CALL(location_manager, remove).Times(4);
	EXPECT_CALL(location_manager, updateLocations());

	// Actual call
	graph.balance(load_balancing, fpmas::api::graph::PARTITION);
}

/*
 * T is either NodePtrWrapper or EdgePtrWrapper.
 *
 * The deserialize result is a temporary variable using internally by GMock
 * matchers, so the pointers contained in returned NodePtrWrapper and
 * EdgePtrWrapper could normally not be deleted, what produces memory leaks. So
 * instead equivalent unique_ptr instances are returned, so that pointers are
 * automatically deleted when the temporary returned value is deleted.
 *
 * The NodePtrWrapper and EdgePtrWrapper PackType specialization are still
 * used.
 */
template<typename T, typename PackType>
std::unordered_map<int, std::vector<std::unique_ptr<typename T::element_type>>> deserialize(
		std::unordered_map<int, DataPack> pack) {
	std::unordered_map<int, std::vector<T>> data_export;
	for(auto& item : pack)
		data_export[item.first] = PackType::parse(item.second)
			.template get<std::vector<T>>();
	std::unordered_map<int, std::vector<std::unique_ptr<typename T::element_type>>> data;
	for(auto item : data_export)
		for(auto ptr : item.second)
			data[item.first].push_back(std::unique_ptr<typename T::element_type>(ptr.get()));
	return data;
}

template<typename PackType, typename T>
std::unordered_map<int, DataPack> serialize(std::unordered_map<int, std::vector<T>> data) {
	std::unordered_map<int, DataPack> pack;
	for(auto& item : data)
		pack[item.first] = PackType(item.second).dump();
	return pack;
}

/*
 * Node distribution test (no edges)
 */
TEST_F(DistributedGraphDistributeTest, distribute_without_link) {
	auto export_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[3])),
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[4]))
					)),
			Pair(1, UnorderedElementsAre(
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[1])),
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[2]))
					))
			);

	// No edge import
	std::unordered_map<int, DataPack> serial_edge_import
		= serialize<fpmas::io::datapack::ObjectPack>(
			std::unordered_map<int, std::vector<EdgePtrWrapper<int>>>());
	std::unordered_map<int, DataPack> edge_arg;
	EXPECT_CALL(comm, allToAll(_, MPI_CHAR))
		.WillOnce(DoAll(
					SaveArg<0>(&edge_arg),
					Return(serial_edge_import)
					));

	// Mock node import
	// mock_nodes are not inserted in the graph but copied to the
	// serial_node_import buffer, so they don't need to be dynamically
	// allocated
	std::array<MockNode, 2> mock_nodes {{{DistributedId(2,5)}, {DistributedId(4, 3)}}};
	std::unordered_map<int, std::vector<NodePtrWrapper<int>>> node_import {
		{2, {&mock_nodes[0], &mock_nodes[1]}}
	};

	std::unordered_map<int, DataPack> serial_node_import
		= serialize<fpmas::io::datapack::ObjectPack>(node_import);
	std::unordered_map<int, DataPack> node_arg;
	EXPECT_CALL(comm, allToAll(
				ResultOf(
					&deserialize<NodePtrWrapper<int>, fpmas::io::datapack::ObjectPack>,
					export_map_matcher
					), MPI_CHAR))
		.WillOnce(DoAll(
					SaveArg<0>(&node_arg),
					Return(serial_node_import)));


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

	EXPECT_CALL(mock_sync_linker, synchronize());
	EXPECT_CALL(mock_data_sync, synchronize());
	graph.distribute(fake_partition);

	ASSERT_THAT(edge_arg, ResultOf(
					&deserialize<EdgePtrWrapper<int>, fpmas::io::datapack::LightObjectPack>,
					IsEmpty()));
	ASSERT_THAT(node_arg, ResultOf(
					&deserialize<NodePtrWrapper<int>, fpmas::io::datapack::ObjectPack>,
					export_map_matcher));

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
class DistributedGraphDistributeWithEdgeTest : public DistributedGraphDistributeTest {
	protected:
		typedef std::vector<fpmas::api::graph::DistributedEdge<int>*> EdgeList;

		EdgeType* edge1;
		EdgeType* edge2;

		EdgeList node2_in;
		EdgeList node3_out;

		MockEventCallback<SetDistantNodeEvent<int>>* distant_callback
			= new MockEventCallback<SetDistantNodeEvent<int>>;
		std::array<NodeType*, 2> imported_edge_distant_callback_args;

		void SetUp() override {
			DistributedGraphDistributeTest::SetUp();

			// No lock to manage, all edges are local
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
			EXPECT_CALL(*distant_callback, call(SetDistantNodeEventContextIs(
						SetDistantNodeEvent<int>::IMPORT_NEW_DISTANT
						)))
				.WillOnce(SaveNode(&imported_edge_distant_callback_args[0]))
				.WillOnce(SaveNode(&imported_edge_distant_callback_args[1]));

			// Exported node callback matchers
			EXPECT_CALL(*distant_callback, call(SetDistantNodeEvent<int>(
							graph.getNode(node_ids[2]),
							SetDistantNodeEvent<int>::EXPORT_DISTANT
							)));
			EXPECT_CALL(*distant_callback, call(SetDistantNodeEvent<int>(
							graph.getNode(node_ids[3]),
							SetDistantNodeEvent<int>::EXPORT_DISTANT
							)));
			EXPECT_CALL(*distant_callback, call(SetDistantNodeEvent<int>(
							graph.getNode(node_ids[1]),
							SetDistantNodeEvent<int>::EXPORT_DISTANT
							)));
			EXPECT_CALL(*distant_callback, call(SetDistantNodeEvent<int>(
							graph.getNode(node_ids[4]),
							SetDistantNodeEvent<int>::EXPORT_DISTANT
							)));

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
TEST_F(DistributedGraphDistributeWithEdgeTest, distribute_with_edge_test) {
	auto export_node_map_matcher = UnorderedElementsAre(
			Pair(0, UnorderedElementsAre(
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[3])),
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[4]))
					)),
			Pair(1, UnorderedElementsAre(
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[1])),
					Pointee(Property(
							&fpmas::api::graph::DistributedNode<int>::getId,
							node_ids[2]))
					))
			);
	
	auto export_edge_map_matcher = UnorderedElementsAre(
		Pair(0, ElementsAre(Pointee(Property(
					&fpmas::api::graph::DistributedEdge<int>::getId,
					edge2->getId()
					)))),
		Pair(1, ElementsAre(Pointee(Property(
					&fpmas::api::graph::DistributedEdge<int>::getId,
					edge1->getId()
					))))
	);

	// edge, mock_src and mock_tgt are not inserted in the graph: they are just
	// copied to the serial_edge_import buffer, so they don't need to be
	// dynamically allocated.
	MockEdge edge(DistributedId(4, 6), 2);
	MockNode mock_src(DistributedId(4, 8));
	edge.setSourceNode(&mock_src);
	MockNode mock_tgt(DistributedId(3, 2));
	edge.setTargetNode(&mock_tgt);

	std::unordered_map<int, std::vector<EdgePtrWrapper<int>>> edge_import {
		{3, {&edge}}
	};

	EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(2);



	// No node import
	std::unordered_map<int, DataPack> node_arg;
	std::unordered_map<int, DataPack> serial_node_import
		= serialize<fpmas::io::datapack::ObjectPack>(
				std::unordered_map<int, std::vector<NodePtrWrapper<int>>>());

	// Mock edge import
	std::unordered_map<int, DataPack> edge_arg;
	std::unordered_map<int, DataPack> serial_edge_import
		= serialize<fpmas::io::datapack::LightObjectPack>(edge_import);

	EXPECT_CALL(comm, allToAll(_, MPI_CHAR))
		// First allToAll call imports nodes
		.WillOnce(DoAll(
					SaveArg<0>(&node_arg),
					Return(serial_node_import)
					))
		// Second allToAll call imports edges
		.WillOnce(DoAll(
					SaveArg<0>(&edge_arg),
					Return(serial_edge_import)
					));

	EXPECT_CALL(location_manager, updateLocations());
	EXPECT_CALL(mock_sync_linker, synchronize());
	EXPECT_CALL(mock_data_sync, synchronize());
	graph.distribute(fake_partition);

	ASSERT_THAT(node_arg, ResultOf(
					&deserialize<NodePtrWrapper<int>, fpmas::io::datapack::ObjectPack>,
					export_node_map_matcher));
	ASSERT_THAT(edge_arg, ResultOf(
					&deserialize<EdgePtrWrapper<int>, fpmas::io::datapack::LightObjectPack>,
					export_edge_map_matcher));
	ASSERT_THAT(
			imported_edge_distant_callback_args,
			UnorderedElementsAre(
				Pointee(Property(&fpmas::api::graph::DistributedNode<int>::getId, mock_src.getId())),
				Pointee(Property(&fpmas::api::graph::DistributedNode<int>::getId, mock_tgt.getId()))
				)
			);
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
				graph.addCallOnInsertNode(new NiceMock<MockCallback<
						fpmas::api::graph::DistributedNode<int>*>
						>);
				graph.addCallOnEraseNode(new NiceMock<MockCallback<
						fpmas::api::graph::DistributedNode<int>*>
						>);
				graph.addCallOnSetLocal(new NiceMock<MockEventCallback<
						SetLocalNodeEvent<int>>
						>);
				graph.addCallOnSetDistant(new NiceMock<MockEventCallback<
						SetDistantNodeEvent<int>>
						>);
				graph.addCallOnInsertEdge(new NiceMock<MockCallback<
						fpmas::api::graph::DistributedEdge<int>*>
						>);
				graph.addCallOnEraseEdge(new NiceMock<MockCallback<
						fpmas::api::graph::DistributedEdge<int>*>
						>);
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
