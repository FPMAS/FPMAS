#include "fpmas.h"

// Imports tests and json serialization rules
#include "json.h"

FPMAS_JSON_SET_UP(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);

// ObjectPack serialization is still possible, even if no ObjectPack
// serialization rules are defined. Falls back to json serialization.
TEST(AgentSerializer, object_pack_fallback) {
	MockAgent<4>* agent_4 = new MockAgent<4>;
	MockAgent<12>* agent_12 = new MockAgent<12>;
	EXPECT_CALL(*agent_4, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_4, getField).WillRepeatedly(Return(7));
	EXPECT_CALL(*agent_4, groupIds).WillRepeatedly(
			Return(std::vector<fpmas::api::model::GroupId> {2, 6, 68}));
	EXPECT_CALL(*agent_12, typeId).Times(AnyNumber());
	EXPECT_CALL(*agent_12, getField).WillRepeatedly(Return(84));
	EXPECT_CALL(*agent_12, groupIds).WillRepeatedly(
			Return(std::vector<fpmas::api::model::GroupId> {4, 90}));

	fpmas::api::model::AgentPtr agent_ptr_4 {agent_4};
	fpmas::api::model::AgentPtr agent_ptr_12 {agent_12};

	fpmas::io::datapack::ObjectPack p4 = agent_ptr_4;
	fpmas::io::datapack::ObjectPack p12 = agent_ptr_12;

	agent_ptr_4 = p4.get<fpmas::api::model::AgentPtr>();
    agent_ptr_12 = p12.get<fpmas::api::model::AgentPtr>();

	ASSERT_THAT(agent_ptr_4.get(), WhenDynamicCastTo<MockAgent<4>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<4>*>(agent_ptr_4.get())->getField(), 7);
	ASSERT_THAT(agent_ptr_4->groupIds(), UnorderedElementsAre(2, 6, 68));
	ASSERT_THAT(agent_ptr_12.get(), WhenDynamicCastTo<MockAgent<12>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<12>*>(agent_ptr_12.get())->getField(), 84);
	ASSERT_THAT(agent_ptr_12->groupIds(), UnorderedElementsAre(4, 90));
}

// LightObjectPack serialization is still possible, even if no LightObjectPack
// serialization rules are defined. Falls back to json serialization.
TEST(AgentSerializer, light_object_pack_fallback) {
	fpmas::model::AgentPtr ptr(new CustomAgentWithLightPack(12));

	fpmas::io::datapack::ObjectPack classic_pack = ptr;
	fpmas::io::datapack::LightObjectPack light_pack = ptr;
	ASSERT_LT(light_pack.data().size, classic_pack.data().size);

	fpmas::model::AgentPtr unserialized_agent = light_pack.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<CustomAgentWithLightPack*>(Not(IsNull()))
			);

	// Ensures that light_json specialization was used
	ASSERT_FLOAT_EQ(
			dynamic_cast<CustomAgentWithLightPack*>(unserialized_agent.get())
			->very_important_data,
			12
			);
}

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);

	std::cout << "Running AgentPtr json only test suit (FPMAS " << FPMAS_VERSION << ")" << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
