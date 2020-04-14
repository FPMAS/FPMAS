#include "gtest/gtest.h"

#include "graph/base/basic_id.h"
#include "api/graph/base/mock_graph_base.h"
#include "api/graph/base/mock_node.h"
#include "api/graph/base/mock_arc.h"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;
using ::testing::AtLeast;

using FPMAS::graph::base::BasicId;

TEST(BasicGraphTest, buildDefaultNode) {
	MockGraphBase<MockNode<MockData, BasicId>, MockArc<MockData, BasicId>> graph;

	BasicId id;
	for (int i = 0; i < 10; ++i) {
		auto node = new MockNode<MockData, BasicId>(++id);
		EXPECT_CALL(*node, getId).Times(AtLeast(2));
		graph.insert(node);

		ASSERT_EQ(graph.getNodes().size(), i+1);
		ASSERT_EQ(graph.getNode(node->getId()), node);
	}
}
/*
 *
 *TEST(BasicGraphTest, buildDataNode) {
 *    BasicGraph<int, BasicId, MockNode, MockArc> graph;
 *
 *    for (int i = 0; i < 10; ++i) {
 *        auto node = graph.buildNode(std::move(i));
 *        EXPECT_CALL(*node, getId).Times(2);
 *        EXPECT_CALL(*node, data()).Times(1);
 *
 *        ASSERT_EQ(graph.getNodes().size(), i+1);
 *        ASSERT_EQ(graph.getNode(node->getId()), node);
 *
 *        ASSERT_EQ(graph.getNode(node->getId())->data(), i);
 *    }
 *}
 *
 *TEST(BasicGraphTest, buildWeightedDataNode) {
 *    BasicGraph<int, BasicId, MockNode, MockArc> graph;
 *
 *    for (int i = 0; i < 10; ++i) {
 *        auto node = graph.buildNode(std::move(i), 1.5 * i);
 *        EXPECT_CALL(*node, getId).Times(3);
 *        EXPECT_CALL(*node, data()).Times(1);
 *        EXPECT_CALL(*node, getWeight()).Times(1);
 *
 *        ASSERT_EQ(graph.getNodes().size(), i+1);
 *        ASSERT_EQ(graph.getNode(node->getId()), node);
 *
 *        ASSERT_EQ(graph.getNode(node->getId())->data(), i);
 *        ASSERT_EQ(graph.getNode(node->getId())->getWeight(), 1.5 * i);
 *    }
 *}
 */
