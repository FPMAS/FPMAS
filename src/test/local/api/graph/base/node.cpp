#include "gtest/gtest.h"

#include "../mocks/graph/base/mock_node.h"

#include "graph/base/basic_id.h"

using FPMAS::graph::base::BasicId;

using ::testing::Contains;
using ::testing::Return;
using ::testing::AtLeast;

TEST(NodeApiTest, in_neighbors_test) {
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

	for(const auto& neighbor : nodes) {
		ASSERT_THAT(neighbors, Contains(&neighbor));
	}
}
