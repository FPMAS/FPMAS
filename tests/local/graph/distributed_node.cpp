#include "fpmas/graph/distributed_node.h"

#include "../../mocks/synchro/mock_mutex.h"
#include "../io/fake_data.h"

using namespace testing;
using fpmas::graph::DistributedNode;
using fpmas::api::graph::LocationState;

class DistributedNodeTest : public Test {
	protected:
		DistributedNode<int> node {{7, 4}, -15};
};

TEST_F(DistributedNodeTest, constructor) {
	ASSERT_EQ(node.getId(), fpmas::graph::DistributedId(7, 4));
	ASSERT_EQ(node.data(), -15);
	// Default weight
	ASSERT_EQ(node.getWeight(), 1.f);
}

TEST_F(DistributedNodeTest, state) {
	node.setState(LocationState::DISTANT);
	ASSERT_EQ(node.state(), LocationState::DISTANT);

	node.setState(LocationState::LOCAL);
	ASSERT_EQ(node.state(), LocationState::LOCAL);
}

TEST_F(DistributedNodeTest, location) {
	node.setLocation(19);
	ASSERT_EQ(node.location(), 19);
}

TEST_F(DistributedNodeTest, mutex) {
	MockMutex<int>* mutex = new MockMutex<int>;

	node.setMutex(mutex);
	ASSERT_EQ(node.mutex(), mutex);

	// mutex must be deleted by node
}

class DistributedNodeSerializationTest : public Test {
	protected:
		DistributedNode<DefaultConstructibleData> node {{2, 6}, {7}};
		fpmas::graph::NodePtrWrapper<DefaultConstructibleData> node_ptr {&node};

		void SetUp() override {
			node.setLocation(3);
			node.setWeight(2.4);
		}

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = node_ptr;
			}

		template<typename PackType>
			void Unserialize(const PackType& pack) {
				node_ptr = pack.template get<decltype(node_ptr)>();
			}

		void TearDown() override {
			ASSERT_EQ(node_ptr->getId(), node.getId());
			delete node_ptr.get();
		}
};

TEST_F(DistributedNodeSerializationTest, json) {
	nlohmann::json classic_json;
	Serialize(classic_json);
	Unserialize(classic_json);

	ASSERT_FLOAT_EQ(node_ptr->getWeight(), node.getWeight());
	ASSERT_EQ(node_ptr.get()->data().i, 7);
}

TEST_F(DistributedNodeSerializationTest, light_json) {
	nlohmann::json classic_json;
	Serialize(classic_json);

	fpmas::io::json::light_json light_json;
	Serialize(light_json);
	Unserialize(light_json);

	ASSERT_LT(light_json.dump().size(), classic_json.dump().size());
}

TEST_F(DistributedNodeSerializationTest, ObjectPack) {
	fpmas::io::datapack::ObjectPack classic_pack;
	Serialize(classic_pack);
	Unserialize(classic_pack);

	ASSERT_FLOAT_EQ(node_ptr->getWeight(), node.getWeight());
	ASSERT_EQ(node_ptr.get()->data().i, 7);
}

TEST_F(DistributedNodeSerializationTest, LightObjectPack) {
	fpmas::io::datapack::ObjectPack classic_pack;
	Serialize(classic_pack);

	fpmas::io::datapack::LightObjectPack light_pack;
	Serialize(light_pack);
	Unserialize(light_pack);

	ASSERT_LT(light_pack.data().size, classic_pack.data().size);
}
