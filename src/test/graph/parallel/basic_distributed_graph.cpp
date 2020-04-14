#include "gtest/gtest.h"

#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/mock_distributed_arc.h"

#include "graph/parallel/basic_distributed_graph.h"

using FPMAS::graph::parallel::BasicDistributedGraph;

TEST(BasicDistributedGraph, buildNode) {
	BasicDistributedGraph<MockDistributedNode<int>, MockDistributedArc<int>> graph;

	auto currentId = graph.currentNodeId();
	auto node = graph.buildNode(2, 0.5, LocationState::LOCAL);

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

TEST(BasicDistributedGraph, local_link) {
	BasicDistributedGraph<MockDistributedNode<int>, MockDistributedArc<int>> graph;

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
