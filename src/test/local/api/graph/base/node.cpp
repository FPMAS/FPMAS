#include "gtest/gtest.h"

#include "../mocks/graph/base/mock_node.h"

#include "graph/base/basic_id.h"

using FPMAS::graph::base::BasicId;

using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::AtLeast;

TEST(NodeApiTest, empty_in_neighbors) {
	MockNode<BasicId> node;
	ASSERT_THAT(node.inNeighbors(), IsEmpty());
	ASSERT_THAT(node.inNeighbors(12), IsEmpty());
}

TEST(NodeApiTest, empty_out_neighbors) {
	MockNode<BasicId> node;
	ASSERT_THAT(node.outNeighbors(), IsEmpty());
	ASSERT_THAT(node.outNeighbors(12), IsEmpty());
}

TEST(NodeApiTest, in_neighbors) {
	// Mock in neighbors nodes
	std::array<MockNode<BasicId>, 10> nodes;
	// Mock arcs
	std::array<MockArc<BasicId>, 10> arcs;

	MockNode<BasicId> node;
	std::vector<MockArc<BasicId>*> inArcs;
	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(arcs[i], getTargetNode).Times(0);
		EXPECT_CALL(arcs[i], getSourceNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(&nodes[i]));
		inArcs.push_back(&arcs[i]);
	}
	EXPECT_CALL(node, getOutgoingArcs()).Times(0);
	EXPECT_CALL(node, getIncomingArcs()).WillOnce(Return(inArcs));

	auto neighbors = node.inNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto& neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(&neighbor));
	}
}

TEST(NodeApiTest, out_neighbors) {
	// Mock in neighbors nodes
	std::array<MockNode<BasicId>, 10> nodes;
	// Mock arcs
	std::array<MockArc<BasicId>, 10> arcs;

	MockNode<BasicId> node;
	std::vector<MockArc<BasicId>*> outArcs;
	for(int i = 0; i < 10; i++) {
		EXPECT_CALL(arcs[i], getSourceNode).Times(0);
		EXPECT_CALL(arcs[i], getTargetNode)
			.Times(AtLeast(1))
			.WillRepeatedly(Return(&nodes[i]));
		outArcs.push_back(&arcs[i]);
	}
	EXPECT_CALL(node, getIncomingArcs()).Times(0);
	EXPECT_CALL(node, getOutgoingArcs()).WillOnce(Return(outArcs));

	auto neighbors = node.outNeighbors();

	ASSERT_THAT(neighbors, SizeIs(10));
	for(const auto& neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(&neighbor));
	}
}
