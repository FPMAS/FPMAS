#include "fpmas/model/serializer.h"
#include "../mocks/model/mock_model.h"
#include "fpmas/utils/macros.h"
#include "gtest_environment.h"

using ::testing::AnyNumber;
using ::testing::WhenDynamicCastTo;
using ::testing::Not;
using ::testing::IsNull;
using ::testing::UnorderedElementsAre;

TEST(AgentSerializer, to_json) {
	MockAgent<4>* agent_4 = new MockAgent<4>;
	MockAgent<12>* agent_12 = new MockAgent<12>;
	EXPECT_CALL(*agent_4, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_4, getField).WillRepeatedly(Return(7));
	EXPECT_CALL(*agent_4, groupIds).WillRepeatedly(
			Return(std::vector<fpmas::api::model::GroupId> {2, 67}));
	EXPECT_CALL(*agent_12, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_12, getField).WillRepeatedly(Return(84));
	EXPECT_CALL(*agent_12, groupIds).WillRepeatedly(
			Return(std::vector<fpmas::api::model::GroupId> {8, 17, 5}));

	fpmas::api::model::AgentPtr agent_ptr_4 {agent_4};
	fpmas::api::model::AgentPtr agent_ptr_12 {agent_12};

	nlohmann::json j4 = agent_ptr_4;
	nlohmann::json j12 = agent_ptr_12;

	ASSERT_EQ(j4.at("type").get<fpmas::api::model::TypeId>(), std::type_index(typeid(MockAgent<4>)));
	ASSERT_THAT(
			j4.at("gids").get<std::vector<fpmas::api::model::GroupId>>(),
			UnorderedElementsAre(2, 67));
	ASSERT_EQ(j4.at("agent").at("mock").get<int>(), 7);

	ASSERT_EQ(j12.at("type").get<fpmas::api::model::TypeId>(), std::type_index(typeid(MockAgent<12>)));
	ASSERT_THAT(
			j12.at("gids").get<std::vector<fpmas::api::model::GroupId>>(),
			UnorderedElementsAre(8, 17, 5));
	ASSERT_EQ(j12.at("agent").at("mock").get<int>(), 84);
}

TEST(AgentSerializer, from_json) {
	nlohmann::json j4;
	j4["type"] = std::type_index(typeid(MockAgent<4>));
	j4["gids"] = {2, 6, 68};
	j4["agent"]["field"] = 7;

	nlohmann::json j12;
	j12["type"] = std::type_index(typeid(MockAgent<12>));
	j12["gids"] = {4, 90};
	j12["agent"]["field"] = 84;

	auto agent_4 = j4.get<fpmas::api::model::AgentPtr>();
	auto agent_12 = j12.get<fpmas::api::model::AgentPtr>();

	ASSERT_THAT(agent_4.get(), WhenDynamicCastTo<MockAgent<4>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<4>*>(agent_4.get())->getField(), 7);
	ASSERT_THAT(agent_4->groupIds(), UnorderedElementsAre(2, 6, 68));
	ASSERT_THAT(agent_12.get(), WhenDynamicCastTo<MockAgent<12>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<12>*>(agent_12.get())->getField(), 84);
	ASSERT_THAT(agent_12->groupIds(), UnorderedElementsAre(4, 90));
}
