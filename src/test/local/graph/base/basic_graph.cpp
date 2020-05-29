#include "gtest/gtest.h"

#include "graph/base/basic_graph.h"
#include "graph/base/basic_id.h"
#include "../mocks/graph/base/mock_node.h"
#include "../mocks/graph/base/mock_arc.h"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;
using ::testing::AtLeast;

using FPMAS::graph::base::BasicId;

template<typename NodeType, typename ArcType>
class MockAbstractGraphBase : 
	public virtual FPMAS::graph::base::AbstractGraphBase<NodeType, ArcType> {
		typedef FPMAS::graph::base::AbstractGraphBase<NodeType, ArcType>
		GraphBase;
		using typename GraphBase::NodeIdType;
		using typename GraphBase::NodeMap;

		using typename GraphBase::ArcIdType;
		using typename GraphBase::ArcMap;

		public:
			MOCK_METHOD(void, removeNode, (NodeType*), (override));
			MOCK_METHOD(void, unlink, (ArcType*), (override));

};
TEST(BasicGraphTest, buildDefaultNode) {
	MockAbstractGraphBase<MockNode<MockData, BasicId>, MockArc<MockData, BasicId>> graph;

	BasicId id;
	for (int i = 0; i < 10; ++i) {
		auto node = new MockNode<MockData, BasicId>(++id);
		EXPECT_CALL(*node, getId).Times(AtLeast(2));
		graph.insert(node);

		ASSERT_EQ(graph.getNodes().size(), i+1);
		ASSERT_EQ(graph.getNode(node->getId()), node);
	}
}

class BasicGraphEraseArcTest : public ::testing::Test {
	protected:
		MockAbstractGraphBase<MockNode<MockData, BasicId>, MockArc<MockData, BasicId>> graph;
		BasicId id;
		MockNode<MockData, BasicId>* src = new MockNode<MockData, BasicId>(++id);
		MockNode<MockData, BasicId>* tgt = new MockNode<MockData, BasicId>(++id);
		MockArc<MockData, BasicId>* arc = new MockArc<MockData, BasicId>(++id, 2);

		void SetUp() override {
			graph.insert(src);
			graph.insert(tgt);
			arc->src = src;
			arc->tgt = tgt;
			graph.insert(arc);
		}
};

TEST_F(BasicGraphEraseArcTest, erase) {
	EXPECT_CALL(*src, unlinkOut(arc));
	EXPECT_CALL(*src, unlinkIn).Times(0);
	EXPECT_CALL(*tgt, unlinkOut).Times(0);
	EXPECT_CALL(*tgt, unlinkIn(arc));

	graph.erase(arc);
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
