#include "fpmas/graph/node.h"
#include "api/graph/basic_id.h"

#include "../mocks/graph/mock_edge.h"
#include "../mocks/utils/mock_callback.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;
using ::testing::ElementsAre;
using ::testing::Contains;

using fpmas::graph::Node;

class BasicMockEdge : public AbstractMockEdge<BasicId, Node<BasicId, BasicMockEdge>> {
};

class NodeTest : public ::testing::Test {
	protected:
		BasicId id;
		Node<BasicId, BasicMockEdge> node {++id};
		Node<BasicId, BasicMockEdge> otherNode {++id};
		BasicMockEdge edge;

		void SetUp() override {
			EXPECT_CALL(edge, getId).WillRepeatedly(Return(++id));
			EXPECT_CALL(edge, getLayer).WillRepeatedly(Return(4));
		}

};

TEST_F(NodeTest, void_incoming_edges) {
	ASSERT_THAT(node.getIncomingEdges(), SizeIs(0));
	ASSERT_THAT(node.getIncomingEdges(12), SizeIs(0));
}

TEST_F(NodeTest, void_outgoing_edges) {
	ASSERT_THAT(node.getOutgoingEdges(), SizeIs(0));
	ASSERT_THAT(node.getOutgoingEdges(12), SizeIs(0));
}

TEST_F(NodeTest, linkIn) {
	node.linkIn(&edge);
	ASSERT_THAT(node.getIncomingEdges(4), SizeIs(1));
	ASSERT_THAT(node.getIncomingEdges(4), ElementsAre(&edge));
}

TEST_F(NodeTest, linkOut) {
	node.linkOut(&edge);
	ASSERT_THAT(node.getOutgoingEdges(4), SizeIs(1));
	ASSERT_THAT(node.getOutgoingEdges(4), ElementsAre(&edge));
}

TEST_F(NodeTest, unlinkIn) {
	node.linkIn(&edge);
	EXPECT_CALL(edge, getSourceNode).WillRepeatedly(Return(&otherNode));
	EXPECT_CALL(edge, getTargetNode).WillRepeatedly(Return(&node));

	node.unlinkIn(&edge);
	ASSERT_THAT(node.getIncomingEdges(4), IsEmpty());
}

TEST_F(NodeTest, unlinkOut) {
	node.linkOut(&edge);
	EXPECT_CALL(edge, getSourceNode).WillRepeatedly(Return(&node));
	EXPECT_CALL(edge, getTargetNode).WillRepeatedly(Return(&otherNode));

	node.unlinkOut(&edge);
	ASSERT_THAT(node.getOutgoingEdges(4), IsEmpty());
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
	std::vector<Node<BasicId, BasicMockEdge>*> nodes;
	for(int i = 0; i < 10; i++)
		nodes.push_back(new Node<BasicId, BasicMockEdge>(++id));

	// Mock edges
	std::array<BasicMockEdge, 10> edges;

	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(edges[i], getTargetNode).Times(0);
		EXPECT_CALL(edges[i], getSourceNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(nodes[i]));
		node.linkIn(&edges[i]);
	}

	auto neighbors = node.inNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto* neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(neighbor));
	}
	for(auto* node : nodes)
		delete node;
}

TEST_F(NodeTest, out_neighbors) {
	// Mock in neighbors nodes
	std::vector<Node<BasicId, BasicMockEdge>*> nodes;
	for(int i = 0; i < 10; i++)
		nodes.push_back(new Node<BasicId, BasicMockEdge>(++id));

	// Mock edges
	std::array<BasicMockEdge, 10> edges;

	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(edges[i], getSourceNode).Times(0);
		EXPECT_CALL(edges[i], getTargetNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(nodes[i]));
		node.linkOut(&edges[i]);
	}

	auto neighbors = node.outNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto* neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(neighbor));
	}
}
