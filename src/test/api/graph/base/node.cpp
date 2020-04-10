#include "gtest/gtest.h"

#include "mock_arc.h"
#include "mock_node.h"

#include "graph/base/basic_id.h"

#include "utils/test.h"

using FPMAS::graph::base::BasicId;
using FPMAS::test::ASSERT_CONTAINS;

using ::testing::Return;
using ::testing::AtLeast;

TEST(NodeApiTest, in_neighbors_test) {
	// Mock in neighbors nodes
	std::array<MockNode<MockData, BasicId>, 10> nodes;
	// Mock arcs
	std::array<MockArc<MockData, BasicId>, 10> arcs;

	MockNode<MockData, BasicId> node;
	std::vector<typename MockNode<MockData, BasicId>::arc_type*> inArcs;
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
		ASSERT_CONTAINS(&neighbor, neighbors);
	}
}
