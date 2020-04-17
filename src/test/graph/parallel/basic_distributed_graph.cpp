#include "gtest/gtest.h"

#include "api/communication/mock_communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/mock_distributed_arc.h"
#include "api/graph/parallel/mock_load_balancing.h"

#include "graph/parallel/basic_distributed_graph.h"

using ::testing::IsEmpty;
using ::testing::_;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::NiceMock;

using FPMAS::graph::parallel::BasicDistributedGraph;

class BasicDistributedGraphTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			MockDistributedNode<int>,
			MockDistributedArc<int>,
			MockMpiCommunicator,
			MockLoadBalancing> graph;

};

TEST_F(BasicDistributedGraphTest, buildNode) {

	auto currentId = graph.currentNodeId();
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

	auto currentId = graph.currentArcId();
	auto arc = FPMAS::api::graph::base::link(graph, &srcMock, &tgtMock, 14, 0.5);

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
			MockMpiCommunicator,
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

class BasicDistributedGraphImportArcTest : public BasicDistributedGraphImportTest {
	protected:
		MockDistributedNode<int>* src;
		MockDistributedNode<int>* tgt;
		MockDistributedArc<int>* importedArc;

	void SetUp() override {
		src = graph.buildNode();
		tgt = graph.buildNode();

		// Mock to serialize and import (not yet in the graph)
		auto mock = arc_mock(
				DistributedId(3, 12),
				src, tgt, // Just copy the ids at serialization
				2, // layer
				2.6, // weight
				LocationState::LOCAL // default value
				);
		importedArcMocks[1].push_back(mock);
	}

	void checkArcData() {
		ASSERT_EQ(graph.getArcs().size(), 1);
		ASSERT_EQ(graph.getArcs().count(DistributedId(3, 12)), 1);
		importedArc = graph.getArc(DistributedId(3, 12));
		ASSERT_EQ(importedArc->getSourceNode(), src);
		ASSERT_EQ(importedArc->getTargetNode(), tgt);
		ASSERT_EQ(importedArc->getLayer(), 2);
		ASSERT_EQ(importedArc->getWeight(), 2.6f);
	}

};

TEST_F(BasicDistributedGraphImportArcTest, import_local_arc) {
	distributeTest();
	checkArcData();
	ASSERT_EQ(importedArc->state(), LocationState::LOCAL);
}

TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_src) {
	EXPECT_CALL(*src, state).WillRepeatedly(Return(LocationState::DISTANT));
	distributeTest();
	checkArcData();
	ASSERT_EQ(importedArc->state(), LocationState::DISTANT);
}

TEST_F(BasicDistributedGraphImportArcTest, import_arc_with_existing_distant_tgt) {
	EXPECT_CALL(*tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	distributeTest();
	checkArcData();
	ASSERT_EQ(importedArc->state(), LocationState::DISTANT);
}
