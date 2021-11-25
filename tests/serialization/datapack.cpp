#include "fpmas.h"
#include "datapack.h"

FPMAS_DATAPACK_SET_UP(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);


// json serialization is still possible, even if no nlohmann::json
// serialization rules are defined. Falls back to ObjectPack serialization.
TEST(AgentSerializer, json_fallback) {
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

	nlohmann::json j4 = agent_ptr_4;
	nlohmann::json j12 = agent_ptr_12;

	agent_ptr_4 = j4.get<fpmas::api::model::AgentPtr>();
    agent_ptr_12 = j12.get<fpmas::api::model::AgentPtr>();

	ASSERT_THAT(agent_ptr_4.get(), WhenDynamicCastTo<MockAgent<4>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<4>*>(agent_ptr_4.get())->getField(), 7);
	ASSERT_THAT(agent_ptr_4->groupIds(), UnorderedElementsAre(2, 6, 68));
	ASSERT_THAT(agent_ptr_12.get(), WhenDynamicCastTo<MockAgent<12>*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<MockAgent<12>*>(agent_ptr_12.get())->getField(), 84);
	ASSERT_THAT(agent_ptr_12->groupIds(), UnorderedElementsAre(4, 90));
}

// light_json serialization is still possible, even if no light_json
// serialization rules are defined. Falls back to LightObjectPack serialization.
TEST(AgentSerializer, light_json_fallback) {
	fpmas::model::AgentPtr ptr(new CustomAgentWithLightPack(12));

	nlohmann::json classic_json = ptr;
	fpmas::io::json::light_json light_json = ptr;
	ASSERT_LT(light_json.dump().size(), classic_json.dump().size());

	fpmas::model::AgentPtr unserialized_agent = light_json.get<fpmas::model::AgentPtr>();

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

	std::cout << "Running AgentPtr datapack only test suit (FPMAS " << FPMAS_VERSION << ")" << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
