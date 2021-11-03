#include "fpmas/graph/distributed_edge.h"

#include "graph/mock_distributed_node.h"
#include "../io/fake_data.h"

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

TEST(DistributedEdge, JsonTemporaryNode) {
	fpmas::graph::DistributedNode<int> node({2, 6}, 12);
	node.setWeight(1.7f);
	node.setLocation(4);
	nlohmann::json j	
		= fpmas::graph::NodePtrWrapper<int>(&node);
	fpmas::graph::TemporaryNode<int, nlohmann::json> temp_node({2, 6}, 4, j);

	ASSERT_EQ(temp_node.getId(), fpmas::graph::DistributedId(2, 6));
	ASSERT_EQ(temp_node.getLocation(), 4);

	auto built_node = temp_node.build();
	ASSERT_EQ(built_node->getId(), temp_node.getId());
	ASSERT_EQ(built_node->location(), temp_node.getLocation());
	// Ensures that the built_node is deserialized from the json
	ASSERT_FLOAT_EQ(built_node->getWeight(), 1.7f);
	ASSERT_EQ(built_node->data(), 12);

	delete built_node;
}

TEST(DistributedEdge, TemporaryNode) {
	fpmas::graph::DistributedNode<int> node({2, 6}, 12);
	node.setWeight(1.7f);
	node.setLocation(4);
	fpmas::io::datapack::ObjectPack p
		= fpmas::graph::NodePtrWrapper<int>(&node);

	fpmas::graph::TemporaryNode<int, decltype(p)> temp_node({2, 6}, 4, p);

	ASSERT_EQ(temp_node.getId(), fpmas::graph::DistributedId(2, 6));
	ASSERT_EQ(temp_node.getLocation(), 4);

	auto built_node = temp_node.build();
	ASSERT_EQ(built_node->getId(), temp_node.getId());
	ASSERT_EQ(built_node->location(), temp_node.getLocation());
	// Ensures that the node is deserialized from the ObjectPack
	ASSERT_FLOAT_EQ(built_node->getWeight(), 1.7f);
	ASSERT_EQ(built_node->data(), 12);

	delete built_node;
}

TEST(DistributedEdge, ObjectPack) {
	using namespace fpmas::io::datapack;

	fpmas::graph::DistributedEdge<DefaultConstructibleData> edge {{2, 6}, 7};
	MockDistributedNode<DefaultConstructibleData, NiceMock> src {{0, 1}, {}, 0.7};
	src.data().i = 4;
	MockDistributedNode<DefaultConstructibleData, NiceMock> tgt {{0, 2}, {}, 2.45};
	tgt.data().i = 13;
	ON_CALL(src, location)
		.WillByDefault(Return(3));
	ON_CALL(tgt, location)
		.WillByDefault(Return(22));

	edge.setWeight(2.4);
	edge.setSourceNode(&src);
	edge.setTargetNode(&tgt);

	auto edge_ptr = fpmas::graph::EdgePtrWrapper<DefaultConstructibleData>(&edge);
	ObjectPack classic_object_pack = edge_ptr;

	edge_ptr = classic_object_pack.get<decltype(edge_ptr)>();
	ASSERT_EQ(edge_ptr->getId(), edge.getId());
	ASSERT_FLOAT_EQ(edge_ptr->getWeight(), edge.getWeight());

	auto temp_src = edge_ptr->getTempSourceNode();
	ASSERT_EQ(temp_src->getId(), src.getId());
	ASSERT_EQ(temp_src->getLocation(), 3);
	auto built_src = temp_src->build();
	ASSERT_EQ(built_src->getId(), src.getId());
	ASSERT_EQ(built_src->location(), 3);

	auto temp_tgt = edge_ptr->getTempTargetNode();
	ASSERT_EQ(temp_tgt->getId(), tgt.getId());
	ASSERT_EQ(temp_tgt->getLocation(), 22);
	auto built_tgt = temp_tgt->build();
	ASSERT_EQ(built_tgt->getId(), tgt.getId());
	ASSERT_EQ(built_tgt->location(), 22);

	delete built_src;
	delete built_tgt;
	delete edge_ptr.get();
}

TEST(DistributedEdge, LightObjectPack) {
	using namespace fpmas::io::datapack;

	fpmas::graph::DistributedEdge<DefaultConstructibleData> edge {{2, 6}, 7};
	MockDistributedNode<DefaultConstructibleData, NiceMock> src {{0, 1}, {}, 0.7};
	src.data().i = 4;
	MockDistributedNode<DefaultConstructibleData, NiceMock> tgt {{0, 2}, {}, 2.45};
	tgt.data().i = 13;
	ON_CALL(src, location)
		.WillByDefault(Return(3));
	ON_CALL(tgt, location)
		.WillByDefault(Return(22));

	edge.setWeight(2.4);
	edge.setSourceNode(&src);
	edge.setTargetNode(&tgt);

	auto edge_ptr = fpmas::graph::EdgePtrWrapper<DefaultConstructibleData>(&edge);
	ObjectPack classic_object_pack = edge_ptr;
	LightObjectPack light_object_pack = edge_ptr;

	ASSERT_LT(light_object_pack.data().size, classic_object_pack.data().size);

	edge_ptr = light_object_pack.get<decltype(edge_ptr)>();
	ASSERT_EQ(edge_ptr->getId(), edge.getId());
	ASSERT_FLOAT_EQ(edge_ptr->getWeight(), edge.getWeight());

	auto temp_src = edge_ptr->getTempSourceNode();
	ASSERT_EQ(temp_src->getId(), src.getId());
	ASSERT_EQ(temp_src->getLocation(), 3);
	auto built_src = temp_src->build();
	ASSERT_EQ(built_src->getId(), src.getId());
	ASSERT_EQ(built_src->location(), 3);

	auto temp_tgt = edge_ptr->getTempTargetNode();
	ASSERT_EQ(temp_tgt->getId(), tgt.getId());
	ASSERT_EQ(temp_tgt->getLocation(), 22);
	auto built_tgt = temp_tgt->build();
	ASSERT_EQ(built_tgt->getId(), tgt.getId());
	ASSERT_EQ(built_tgt->location(), 22);

	delete built_src;
	delete built_tgt;
	delete edge_ptr.get();
}
