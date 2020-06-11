#include "graph/base/node.h"
#include "graph/base/basic_id.h"

#include "../mocks/graph/base/mock_arc.h"
#include "../mocks/utils/mock_callback.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::ElementsAre;

using FPMAS::graph::base::Node;
using FPMAS::graph::base::BasicId;

class BasicMockArc : public AbstractMockArc<BasicId, Node<BasicId, BasicMockArc>> {
};

class NodeTest : public ::testing::Test {
	protected:
		BasicId id;
		Node<BasicId, BasicMockArc> node {++id};
		Node<BasicId, BasicMockArc> otherNode {++id};
		BasicMockArc arc;

		void SetUp() override {
			EXPECT_CALL(arc, getId).WillRepeatedly(Return(++id));
			EXPECT_CALL(arc, getLayer).WillRepeatedly(Return(4));
		}

};

TEST_F(NodeTest, void_incoming_arcs) {
	ASSERT_THAT(node.getIncomingArcs(), SizeIs(0));
	ASSERT_THAT(node.getIncomingArcs(12), SizeIs(0));
}

TEST_F(NodeTest, void_outgoing_arcs) {
	ASSERT_THAT(node.getOutgoingArcs(), SizeIs(0));
	ASSERT_THAT(node.getOutgoingArcs(12), SizeIs(0));
}

TEST_F(NodeTest, linkIn) {
	node.linkIn(&arc);
	ASSERT_THAT(node.getIncomingArcs(4), SizeIs(1));
	ASSERT_THAT(node.getIncomingArcs(4), ElementsAre(&arc));
}

TEST_F(NodeTest, linkOut) {
	node.linkOut(&arc);
	ASSERT_THAT(node.getOutgoingArcs(4), SizeIs(1));
	ASSERT_THAT(node.getOutgoingArcs(4), ElementsAre(&arc));
}

TEST_F(NodeTest, unlinkIn) {
	node.linkIn(&arc);
	EXPECT_CALL(arc, getSourceNode).WillRepeatedly(Return(&otherNode));
	EXPECT_CALL(arc, getTargetNode).WillRepeatedly(Return(&node));

	node.unlinkIn(&arc);
	ASSERT_THAT(node.getIncomingArcs(4), IsEmpty());
}

TEST_F(NodeTest, unlinkOut) {
	node.linkOut(&arc);
	EXPECT_CALL(arc, getSourceNode).WillRepeatedly(Return(&node));
	EXPECT_CALL(arc, getTargetNode).WillRepeatedly(Return(&otherNode));

	node.unlinkOut(&arc);
	ASSERT_THAT(node.getOutgoingArcs(4), IsEmpty());
}

/*
 *class NodeCallbackTest : public ::testing::Test {
 *    protected:
 *        Node<BasicId, BasicMockArc> node {BasicId()};
 *
 *};
 *
 *TEST_F(NodeCallbackTest, insert) {
 *    MockCallback* insert = new MockCallback;
 *    node.onInsert(insert);
 *    ASSERT_EQ(node.onInsert(), insert);
 *
 *    MockCallback* other_insert = new MockCallback;
 *    node.onInsert(other_insert);
 *    ASSERT_EQ(node.onInsert(), other_insert);
 *}
 *
 *TEST_F(NodeCallbackTest, erase) {
 *    MockCallback* erase = new MockCallback;
 *    node.onErase(erase);
 *    ASSERT_EQ(node.onErase(), erase);
 *
 *    MockCallback* other_erase = new MockCallback;
 *    node.onErase(other_erase);
 *    ASSERT_EQ(node.onErase(), other_erase);
 *}
 */
