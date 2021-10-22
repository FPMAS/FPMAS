#include "fpmas/graph/distributed_edge.h"

#include "graph/mock_distributed_node.h"

using namespace testing;

TEST(DistributedEdgeConstructorTest, default_weight) {
	fpmas::graph::DistributedEdge<int> edge {{2, 6}, 7};
	ASSERT_FLOAT_EQ(edge.getWeight(), 1.f);
	ASSERT_EQ(edge.getId(), DistributedId(2, 6));
	ASSERT_EQ(edge.getLayer(), 7);
}

class DistributedEdgeTest : public Test {
	protected:
		fpmas::graph::DistributedEdge<int> edge {{2, 6}, 7};
		MockDistributedNode<int, NiceMock> src {{0, 1}, 4, 0.7};
		MockDistributedNode<int, NiceMock> tgt {{0, 2}, 13, 2.45};

		void SetUp() override {
			edge.setWeight(2.4);
			edge.setSourceNode(&src);
			edge.setTargetNode(&tgt);
		}
};

TEST_F(DistributedEdgeTest, state) {
	edge.setState(LocationState::DISTANT);
	ASSERT_EQ(edge.state(), LocationState::DISTANT);

	edge.setState(LocationState::LOCAL);
	ASSERT_EQ(edge.state(), LocationState::LOCAL);
}

TEST(JsonTemporaryNode, test) {
	nlohmann::json j = {{{"id", {2, 6}}, {"weight", 1.7f}, {"data", 12}}, 4};
	fpmas::graph::JsonTemporaryNode<int, nlohmann::json> temp_node({2, 6}, 4, j);

	ASSERT_EQ(temp_node.getId(), fpmas::graph::DistributedId(2, 6));
	ASSERT_EQ(temp_node.getLocation(), 4);

	auto node = temp_node.build();
	ASSERT_EQ(node->getId(), temp_node.getId());
	ASSERT_EQ(node->location(), temp_node.getLocation());
	// Ensures that the node is deserialize from the json
	ASSERT_FLOAT_EQ(node->getWeight(), 1.7f);
	ASSERT_EQ(node->data(), 12);

	delete node;
}
