#include "agent.h"
#include "model/mock_model.h"

using namespace testing;

namespace nlohmann {
	using fpmas::api::utils::PtrWrapper;

	template<int FOO>
	using MockAgentPtr = PtrWrapper<MockAgent<FOO>>;

	template<int FOO>
		struct adl_serializer<MockAgentPtr<FOO>> {
			template<typename JsonType>
			static void to_json(JsonType& j, const MockAgentPtr<FOO>& data) {
				j["field"] = data->getField();
			}

			template<typename JsonType>
			static void from_json(const JsonType& j, MockAgentPtr<FOO>& ptr) {
				ptr = MockAgentPtr<FOO>(new MockAgent<FOO>(j.at("field").template get<int>()));
			}
		};

	template<>
		struct adl_serializer<PtrWrapper<CustomAgent>> {
			static void to_json(json& j, const PtrWrapper<CustomAgent>& agent) {
				j = agent->getData();
			}

			static PtrWrapper<CustomAgent> from_json(const json& j) {
				return new CustomAgent(j.get<float>());
			}
		};

	template<>
		struct adl_serializer<PtrWrapper<DefaultConstructibleCustomAgent>> {
			static void to_json(json& j, const PtrWrapper<DefaultConstructibleCustomAgent>& agent) {
				j = agent->getData();
			}
			static PtrWrapper<DefaultConstructibleCustomAgent> from_json(const json& j) {
				return new DefaultConstructibleCustomAgent(j.get<float>());
			}
		};

	template<>
		struct adl_serializer<PtrWrapper<CustomAgentWithLightPack>> {
			static void to_json(json& j, const PtrWrapper<CustomAgentWithLightPack>& agent) {
				j = {agent->getData(), agent->very_important_data};
			}

			static PtrWrapper<CustomAgentWithLightPack> from_json(const json& j) {
				return new CustomAgentWithLightPack(j[0].get<float>(), j[1].get<int>());
			}

		};
}

namespace fpmas { namespace io { namespace json {
	template<>
	struct light_serializer<PtrWrapper<CustomAgentWithLightPack>> {
		static void to_json(light_json& j, const PtrWrapper<CustomAgentWithLightPack>& agent) {
			j = agent->very_important_data;
		}

		static PtrWrapper<CustomAgentWithLightPack> from_json(const light_json&j) {
			return new CustomAgentWithLightPack(j.get<int>());
		}
	};

}}};

TEST(AgentSerializer, json) {
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

// light_json with default constructible agent (no explicit light_serializer
// specialization)
TEST(AgentSerializer, light_json_default_constructible) {
	fpmas::model::AgentPtr ptr(new DefaultConstructibleCustomAgent);

	nlohmann::json classic_json = ptr;
	fpmas::io::json::light_json light_json = ptr;

	ASSERT_LT(light_json.dump().size(), classic_json.dump().size());

	fpmas::model::AgentPtr unserialized_agent = light_json.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<DefaultConstructibleCustomAgent*>(Not(IsNull()))
			);
}

// light_json with non default constructible agent (no explicit light_serializer
// specialization). Falls back to nlohmann::json.
TEST(AgentSerializer, light_json_not_default_constructible) {
	fpmas::model::AgentPtr ptr(new CustomAgent(4.2f));

	fpmas::io::json::light_json light_json = ptr;
	fpmas::model::AgentPtr unserialized_agent = light_json.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<CustomAgent*>(Not(IsNull()))
			);
}

// User defined light_json specialization
TEST(AgentSerializer, custom_light_json) {
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

