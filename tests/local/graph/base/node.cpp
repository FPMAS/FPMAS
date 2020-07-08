#include "fpmas/graph/base/node.h"
#include "api/graph/base/basic_id.h"

#include "../mocks/graph/base/mock_arc.h"
#include "../mocks/utils/mock_callback.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::ElementsAre;
using ::testing::Contains;

using fpmas::graph::base::Node;

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

TEST_F(NodeTest, empty_in_neighbors) {
	ASSERT_THAT(node.inNeighbors(), IsEmpty());
	ASSERT_THAT(node.inNeighbors(12), IsEmpty());
}

TEST_F(NodeTest, empty_out_neighbors) {
	ASSERT_THAT(node.outNeighbors(), IsEmpty());
	ASSERT_THAT(node.outNeighbors(12), IsEmpty());
}

TEST_F(NodeTest, in_neighbors) {
	// Mock in neighbors nodes
	std::array<Node<BasicId, BasicMockArc>, 10> nodes {
		++id, ++id, ++id, ++id, ++id,
		++id, ++id, ++id, ++id, ++id
	};
	// Mock arcs
	std::array<BasicMockArc, 10> arcs;

	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(arcs[i], getTargetNode).Times(0);
		EXPECT_CALL(arcs[i], getSourceNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(&nodes[i]));
		node.linkIn(&arcs[i]);
	}

	auto neighbors = node.inNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto& neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(&neighbor));
	}
}

TEST_F(NodeTest, out_neighbors) {
	// Mock in neighbors nodes
	std::array<Node<BasicId, BasicMockArc>, 10> nodes {
		++id, ++id, ++id, ++id, ++id,
		++id, ++id, ++id, ++id, ++id
	};
	// Mock arcs
	std::array<BasicMockArc, 10> arcs;

	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(arcs[i], getSourceNode).Times(0);
		EXPECT_CALL(arcs[i], getTargetNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(&nodes[i]));
		node.linkOut(&arcs[i]);
	}

	auto neighbors = node.outNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto& neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(&neighbor));
	}
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
