#include "fpmas/graph/parallel/distributed_arc.h"

#include "../mocks/graph/parallel/mock_distributed_node.h"

class DistributedArcTest : public ::testing::Test {
	protected:
		fpmas::graph::parallel::DistributedArc<int> arc {{2, 6}, 7, 2.4};
		MockDistributedNode<int> src {{0, 1}, 0, 0};
		MockDistributedNode<int> tgt {{0, 2}, 0, 0};

		void SetUp() override {
			arc.setSourceNode(&src);
			arc.setTargetNode(&tgt);
		}
};

TEST_F(DistributedArcTest, state) {
	arc.setState(LocationState::DISTANT);
	ASSERT_EQ(arc.state(), LocationState::DISTANT);

	arc.setState(LocationState::LOCAL);
	ASSERT_EQ(arc.state(), LocationState::LOCAL);
}

TEST_F(DistributedArcTest, json_serialization) {
	EXPECT_CALL(src, getLocation).Times(AnyNumber()).WillRepeatedly(Return(3));
	EXPECT_CALL(tgt, getLocation).Times(AnyNumber()).WillRepeatedly(Return(22));

	nlohmann::json arc_json = fpmas::graph::parallel::ArcPtrWrapper<int>(&arc);

	ASSERT_EQ(arc_json.at("id").get<DistributedId>(), DistributedId(2, 6));
	ASSERT_EQ(arc_json.at("layer").get<LayerId>(), 7);
	ASSERT_FLOAT_EQ(arc_json.at("weight").get<float>(), 2.4);
	ASSERT_EQ(arc_json.at("src_loc").get<int>(), 3);
	ASSERT_EQ(arc_json.at("tgt_loc").get<int>(), 22);

	nlohmann::json src_json = src;
	ASSERT_EQ(arc_json.at("src"), src);

	nlohmann::json tgt_json = tgt;
	ASSERT_EQ(arc_json.at("tgt"), tgt);
}

TEST_F(DistributedArcTest, json_deserialization) {
	nlohmann::json arc_json {
		{"id", DistributedId(2, 6)},
			{"layer", 7},
			{"weight", 2.4},
			{"src_loc", 3},
			{"src", src},
			{"tgt_loc", 22},
			{"tgt", tgt}
	};

	auto arc = arc_json.get<fpmas::graph::parallel::ArcPtrWrapper<int>>();
	ASSERT_EQ(arc->getId(), DistributedId(2, 6));
	ASSERT_EQ(arc->getLayer(), 7);
	ASSERT_FLOAT_EQ(arc->getWeight(), 2.4);

	ASSERT_EQ(arc->getSourceNode()->getId(), DistributedId(0, 1));
	ASSERT_FLOAT_EQ(arc->getSourceNode()->getWeight(), 0);
	ASSERT_EQ(arc->getSourceNode()->data(), 0);
	ASSERT_EQ(arc->getSourceNode()->getLocation(), 3);

	ASSERT_EQ(arc->getTargetNode()->getId(), DistributedId(0, 2));
	ASSERT_FLOAT_EQ(arc->getTargetNode()->getWeight(), 0);
	ASSERT_EQ(arc->getTargetNode()->data(), 0);
	ASSERT_EQ(arc->getTargetNode()->getLocation(), 22);

	delete arc->getSourceNode();
	delete arc->getTargetNode();
	delete arc.get();
}
