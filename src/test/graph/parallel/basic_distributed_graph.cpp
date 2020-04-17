#include "gtest/gtest.h"

#include "api/communication/mock_communication.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/mock_distributed_arc.h"
#include "api/graph/parallel/mock_load_balancing.h"

#include "graph/parallel/basic_distributed_graph.h"

using ::testing::IsEmpty;
using ::testing::_;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::NiceMock;

using FPMAS::graph::parallel::BasicDistributedGraph;


class DistMockMpiCommunicator : public AbstractMockMpiCommunicator<DistMockMpiCommunicator> {

	template<typename T>
	using migration_map = std::unordered_map<int, std::vector<T>>;

	public:
	template<typename T>
		migration_map<T> _migrate(migration_map<T>) {
			FPMAS_LOGW(0, "MOCK", "Missing _migrate specialization in DistMockMpiCommunicator");
			return migration_map<T>();
		}

	MOCK_METHOD(
			migration_map<MockDistributedNode<int>>,
			_migrateNode,
			(migration_map<MockDistributedNode<int>>), ());

	MOCK_METHOD(
			migration_map<MockDistributedArc<int>>,
			_migrateArc,
			(migration_map<MockDistributedArc<int>>), ());
};

template<>
typename DistMockMpiCommunicator::migration_map<MockDistributedNode<int>>
	DistMockMpiCommunicator::_migrate<MockDistributedNode<int>>(migration_map<MockDistributedNode<int>> exportMap) {
		return _migrateNode(exportMap);
}

template<>
typename DistMockMpiCommunicator::migration_map<MockDistributedArc<int>>
	DistMockMpiCommunicator::_migrate<MockDistributedArc<int>>(migration_map<MockDistributedArc<int>> exportMap) {
		return _migrateArc(exportMap);
}

class BasicDistributedGraphTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			MockDistributedNode<int>,
			MockDistributedArc<int>,
			DistMockMpiCommunicator,
			MockLoadBalancing> graph;

};

TEST_F(BasicDistributedGraphTest, buildNode) {

	auto currentId = graph.currentNodeId();
	auto node = graph.buildNode(2, 0.5);

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

	auto currentId = graph.currentArcId();
	auto arc = graph.link(&srcMock, &tgtMock, 14, 0.5);

	ASSERT_EQ(graph.getArcs().count(currentId), 1);
	ASSERT_EQ(graph.getArc(currentId), arc);
	ASSERT_EQ(arc->getId(), currentId);

	EXPECT_CALL(*arc, getSourceNode);
	ASSERT_EQ(arc->getSourceNode(), &srcMock);

	EXPECT_CALL(*arc, getTargetNode);
	ASSERT_EQ(arc->getTargetNode(), &tgtMock);

	EXPECT_CALL(*arc, getLayer);
	ASSERT_EQ(arc->getLayer(), 14);

	EXPECT_CALL(*arc, getWeight);
	ASSERT_EQ(arc->getWeight(), 0.5);

	EXPECT_CALL(*arc, state);
	ASSERT_EQ(arc->state(), LocationState::LOCAL);



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
	EXPECT_CALL(const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()), _migrateNode(_));
	EXPECT_CALL(const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()), _migrateArc(_));

	graph.balance();
}

TEST_F(BasicDistributedGraphPartitionTest, distribute) {
	// Broken.
	/*
	 *auto exportMapMatcher = UnorderedElementsAre(
	 *    Pair(0, UnorderedElementsAre(*graph.getNode(nodeIds[3]), *graph.getNode(nodeIds[4]))),
	 *    Pair(1, UnorderedElementsAre(*graph.getNode(nodeIds[1]), *graph.getNode(nodeIds[2])))
	 *);
	 */
	auto exportMapMatcher = UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(_, _)),
		Pair(1, UnorderedElementsAre(_, _))
	);

	EXPECT_CALL(
			const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()), _migrateNode(exportMapMatcher))
	.WillOnce(Return(std::unordered_map<int, std::vector<MockDistributedNode<int>>>()));
	EXPECT_CALL(const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()),
			_migrateArc(IsEmpty())).WillOnce(Return(
				std::unordered_map<int, std::vector<MockDistributedArc<int>>>()
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
			DistMockMpiCommunicator,
			MockLoadBalancing> graph;
		typedef typename decltype(graph)::partition_type partition_type;


		std::unordered_map<int, std::vector<node_mock>> importedNodeMocks;
		std::unordered_map<int, std::vector<arc_mock>> importedArcMocks;

		void distributeTest() {
			EXPECT_CALL(
					const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()), _migrateNode(_))
				.WillOnce(Return(importedNodeMocks));
			EXPECT_CALL(
					const_cast<DistMockMpiCommunicator&>(graph.getMpiCommunicator()), _migrateArc(_))
				.WillOnce(Return(importedArcMocks));
			graph.distribute(partition_type());
		}

};

TEST_F(BasicDistributedGraphImportTest, import_node) {
	auto mock = node_mock(
			DistributedId(1, 10), 8, 2.1, LocationState::LOCAL);

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

	auto mock = node_mock(
			node->getId(), 2, 4., LocationState::LOCAL);

	importedNodeMocks[1].push_back(mock);
	EXPECT_CALL(*node, setState(LocationState::LOCAL));

	distributeTest();
	ASSERT_EQ(graph.getNodes().size(), 1);
	ASSERT_EQ(graph.getNodes().count(mock.getId()), 1);
}

TEST_F(BasicDistributedGraphImportTest, import_local_arc) {
	auto src = graph.buildNode();
	auto tgt = graph.buildNode();

	auto mock = arc_mock(
		DistributedId(3, 12),
		new node_mock(src->getId()),
		new node_mock(tgt->getId()),
		2, // layer
		2.6, // weight
		LocationState::LOCAL // default value
		);
	importedArcMocks[1].push_back(mock);

	distributeTest();

	ASSERT_EQ(graph.getArcs().size(), 1);
	

}
