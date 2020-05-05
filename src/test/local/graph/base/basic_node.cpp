#include "graph/base/basic_node.h"
#include "graph/base/basic_id.h"

#include "../mocks/graph/base/mock_arc.h"

using FPMAS::graph::base::BasicNode;
using FPMAS::graph::base::BasicId;

class BasicMockArc : public AbstractMockArc<int, BasicId, BasicNode<int, BasicId, BasicMockArc>> {
};

class BasicNodeTest : public ::testing::Test {
	protected:
		BasicId id;
		BasicNode<int, BasicId, BasicMockArc> node {++id};
		BasicNode<int, BasicId, BasicMockArc> otherNode {++id};
		BasicMockArc arc;

		void SetUp() override {
			EXPECT_CALL(arc, getId).WillRepeatedly(Return(++id));
			EXPECT_CALL(arc, getLayer).WillRepeatedly(Return(4));
		}

};

TEST_F(BasicNodeTest, linkIn) {
	node.linkIn(&arc);
	ASSERT_EQ(node.getIncomingArcs(4).size(), 1);
	ASSERT_EQ(node.getIncomingArcs(4)[0], &arc);
}

TEST_F(BasicNodeTest, linkOut) {
	node.linkOut(&arc);
	ASSERT_EQ(node.getOutgoingArcs(4).size(), 1);
	ASSERT_EQ(node.getOutgoingArcs(4)[0], &arc);
}

TEST_F(BasicNodeTest, unlinkIn) {
	node.linkIn(&arc);
	EXPECT_CALL(arc, getSourceNode).WillRepeatedly(Return(&otherNode));
	EXPECT_CALL(arc, getTargetNode).WillRepeatedly(Return(&node));

	node.unlinkIn(&arc);
	ASSERT_EQ(node.getIncomingArcs(4).size(), 0);
}

TEST_F(BasicNodeTest, unlinkOut) {
	node.linkOut(&arc);
	EXPECT_CALL(arc, getSourceNode).WillRepeatedly(Return(&node));
	EXPECT_CALL(arc, getTargetNode).WillRepeatedly(Return(&otherNode));

	node.unlinkOut(&arc);
	ASSERT_EQ(node.getOutgoingArcs(4).size(), 0);
}
