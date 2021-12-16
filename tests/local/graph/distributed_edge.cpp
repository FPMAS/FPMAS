#include "fpmas/graph/distributed_edge.h"

#include "graph/mock_distributed_node.h"
#include "../io/fake_data.h"

using namespace testing;



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

TEST_F(DistributedEdgeTest, constructor) {
	fpmas::graph::DistributedEdge<int> edge {{2, 6}, 7};
	ASSERT_EQ(edge.getId(), DistributedId(2, 6));
	ASSERT_EQ(edge.getLayer(), 7);
	// Default weight
	ASSERT_FLOAT_EQ(edge.getWeight(), 1.f);
}

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

class DistributedEdgeSerializationTest : public Test {
	protected:
		fpmas::graph::DistributedEdge<DefaultConstructibleData> edge {{2, 6}, 7};
		MockDistributedNode<DefaultConstructibleData, NiceMock> src {{0, 1}, {}, 0.7};
		MockDistributedNode<DefaultConstructibleData, NiceMock> tgt {{0, 2}, {}, 2.45};

		fpmas::graph::EdgePtrWrapper<DefaultConstructibleData> edge_ptr {&edge};

		void SetUp() override {
			src.data().i = 4;
			tgt.data().i = 13;
			ON_CALL(src, location)
				.WillByDefault(Return(3));
			ON_CALL(tgt, location)
				.WillByDefault(Return(22));

			edge.setWeight(2.4);
			edge.setSourceNode(&src);
			edge.setTargetNode(&tgt);
		}

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = edge_ptr;
			}

		template<typename PackType>
			void Unserialize(const PackType& pack) {
				edge_ptr = pack.template get<decltype(edge_ptr)>();
			}

		void TearDown() override {
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
};

TEST_F(DistributedEdgeSerializationTest, json) {
	nlohmann::json classic_json;
	Serialize(classic_json);
	Unserialize(classic_json);
}

TEST_F(DistributedEdgeSerializationTest, light_json) {
	nlohmann::json classic_json;
	Serialize(classic_json);

	fpmas::io::json::light_json j;
	Serialize(j);
	Unserialize(j);

	ASSERT_LT(j.dump().size(), classic_json.dump().size());
}

TEST_F(DistributedEdgeSerializationTest, ObjectPack) {
	fpmas::io::datapack::ObjectPack classic_pack;
	Serialize(classic_pack);
	Unserialize(classic_pack);
}

TEST_F(DistributedEdgeSerializationTest, LightObjectPack) {
	fpmas::io::datapack::ObjectPack classic_pack;
	Serialize(classic_pack);

	fpmas::io::datapack::LightObjectPack light_pack;
	Serialize(light_pack);
	Unserialize(light_pack);

	ASSERT_LT(light_pack.data().size, classic_pack.data().size);
}
