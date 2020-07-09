#include "fpmas/graph/distributed_edge.h"

#include "../mocks/graph/mock_distributed_node.h"

class DistributedEdgeTest : public ::testing::Test {
	protected:
		fpmas::graph::DistributedEdge<int> edge {{2, 6}, 7, 2.4};
		MockDistributedNode<int> src {{0, 1}, 0, 0};
		MockDistributedNode<int> tgt {{0, 2}, 0, 0};

		void SetUp() override {
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

TEST_F(DistributedEdgeTest, json_serialization) {
	EXPECT_CALL(src, getLocation).Times(AnyNumber()).WillRepeatedly(Return(3));
	EXPECT_CALL(tgt, getLocation).Times(AnyNumber()).WillRepeatedly(Return(22));

	nlohmann::json edge_json = fpmas::graph::EdgePtrWrapper<int>(&edge);

	ASSERT_EQ(edge_json.at("id").get<DistributedId>(), DistributedId(2, 6));
	ASSERT_EQ(edge_json.at("layer").get<LayerId>(), 7);
	ASSERT_FLOAT_EQ(edge_json.at("weight").get<float>(), 2.4);
	ASSERT_EQ(edge_json.at("src_loc").get<int>(), 3);
	ASSERT_EQ(edge_json.at("tgt_loc").get<int>(), 22);

	nlohmann::json src_json = src;
	ASSERT_EQ(edge_json.at("src"), src);

	nlohmann::json tgt_json = tgt;
	ASSERT_EQ(edge_json.at("tgt"), tgt);
}

TEST_F(DistributedEdgeTest, json_deserialization) {
	nlohmann::json edge_json {
		{"id", DistributedId(2, 6)},
			{"layer", 7},
			{"weight", 2.4},
			{"src_loc", 3},
			{"src", src},
			{"tgt_loc", 22},
			{"tgt", tgt}
	};

	auto edge = edge_json.get<fpmas::graph::EdgePtrWrapper<int>>();
	ASSERT_EQ(edge->getId(), DistributedId(2, 6));
	ASSERT_EQ(edge->getLayer(), 7);
	ASSERT_FLOAT_EQ(edge->getWeight(), 2.4);

	ASSERT_EQ(edge->getSourceNode()->getId(), DistributedId(0, 1));
	ASSERT_FLOAT_EQ(edge->getSourceNode()->getWeight(), 0);
	ASSERT_EQ(edge->getSourceNode()->data(), 0);
	ASSERT_EQ(edge->getSourceNode()->getLocation(), 3);

	ASSERT_EQ(edge->getTargetNode()->getId(), DistributedId(0, 2));
	ASSERT_FLOAT_EQ(edge->getTargetNode()->getWeight(), 0);
	ASSERT_EQ(edge->getTargetNode()->data(), 0);
	ASSERT_EQ(edge->getTargetNode()->getLocation(), 22);

	delete edge->getSourceNode();
	delete edge->getTargetNode();
	delete edge.get();
}
