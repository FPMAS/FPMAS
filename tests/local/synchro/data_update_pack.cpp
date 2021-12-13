#include "fpmas/synchro/data_update_pack.h"

#include "gmock/gmock.h"

using namespace ::testing;

class UpdatePackTest : public Test {
	protected:
		fpmas::api::graph::DistributedId id {2, 47};
		std::string str {"hello_world"};
};

class DataUpdatePackTest : public UpdatePackTest {
	protected:
		fpmas::synchro::DataUpdatePack<std::string> pack {id, str};

};

TEST_F(DataUpdatePackTest, json) {
	nlohmann::json j = pack;
	pack = j.get<decltype(pack)>();

	ASSERT_EQ(pack.id, id);
	ASSERT_EQ(pack.updated_data, str);
}

TEST_F(DataUpdatePackTest, ObjectPack) {
	fpmas::io::datapack::ObjectPack p = pack;
	pack = p.get<decltype(pack)>();

	ASSERT_EQ(pack.id, id);
	ASSERT_EQ(pack.updated_data, str);
}

class NodeUpdatePackTest : public UpdatePackTest {
	protected:
		float weight = 37.45f;
		fpmas::synchro::NodeUpdatePack<std::string> pack {id, str, weight};


};

TEST_F(NodeUpdatePackTest, json) {
	nlohmann::json j = pack;
	pack = j.get<decltype(pack)>();

	ASSERT_EQ(pack.id, id);
	ASSERT_EQ(pack.updated_data, str);
	ASSERT_EQ(pack.updated_weight, weight);
}

TEST_F(NodeUpdatePackTest, ObjectPack) {
	fpmas::io::datapack::ObjectPack p = pack;
	pack = p.get<decltype(pack)>();

	ASSERT_EQ(pack.id, id);
	ASSERT_EQ(pack.updated_data, str);
	ASSERT_EQ(pack.updated_weight, weight);
}

