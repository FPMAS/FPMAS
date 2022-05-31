#include "agent.h"
#include "model/mock_model.h"

using namespace testing;

namespace fpmas { namespace io { namespace datapack {
	template<int FOO>
	using MockAgentPtr = fpmas::api::utils::PtrWrapper<MockAgent<FOO>>;

	template<int FOO>
		struct Serializer<MockAgentPtr<FOO>> {
			template<typename PackType>
				static std::size_t size(const PackType& p, const MockAgentPtr<FOO>& data) {
					return p.size(data->getField());
				}

			template<typename PackType>
				static void to_datapack(PackType& p, const MockAgentPtr<FOO>& data) {
					p.put(data->getField());
				}

			template<typename PackType>
				static MockAgentPtr<FOO> from_datapack(const PackType& p) {
					return new MockAgent<FOO>(p.template get<int>());
				}
		};

	template<>
		struct Serializer<PtrWrapper<CustomAgent>> {
			static std::size_t size(const ObjectPack& p, const PtrWrapper<CustomAgent>& agent) {
				return p.size(agent->getData());
			}

			static void to_datapack(ObjectPack& p, const PtrWrapper<CustomAgent>& agent) {
				p.put(agent->getData());
			}

			static PtrWrapper<CustomAgent> from_datapack(const ObjectPack& p) {
				return new CustomAgent(p.get<float>());
			}

		};

	template<>
		struct Serializer<PtrWrapper<DefaultConstructibleCustomAgent>> {
			static std::size_t size(const ObjectPack& p, const PtrWrapper<DefaultConstructibleCustomAgent>& agent) {
				return p.size(agent->getData());
			}

			static void to_datapack(ObjectPack& p, const PtrWrapper<DefaultConstructibleCustomAgent>& agent) {
				p.put(agent->getData());
			}

			static PtrWrapper<DefaultConstructibleCustomAgent> from_datapack(const ObjectPack& p) {
				return new DefaultConstructibleCustomAgent(p.get<float>());
			}

		};

	template<>
		struct Serializer<PtrWrapper<CustomAgentWithLightPack>> {
			static std::size_t size(const ObjectPack& p, const PtrWrapper<CustomAgentWithLightPack>& agent) {
				return p.size(agent->getData()) + p.size(agent->very_important_data);
			}
			static void to_datapack(ObjectPack& p, const PtrWrapper<CustomAgentWithLightPack>& agent) {
				p.put(agent->getData());
				p.put(agent->very_important_data);
			}

			static PtrWrapper<CustomAgentWithLightPack> from_datapack(const ObjectPack& p) {
				// Call order guaranteed, DO NOT CALL gets FROM THE CONSTRUCTOR
				float f = p.get<float>();
				int i = p.get<int>();
				return new CustomAgentWithLightPack(f, i);
			}
		};

	template<>
		struct LightSerializer<PtrWrapper<CustomAgentWithLightPack>> {
			static std::size_t size(const LightObjectPack& p, const PtrWrapper<CustomAgentWithLightPack>& agent) {
				return p.size(agent->very_important_data);
			}

			static void to_datapack(LightObjectPack& p, const PtrWrapper<CustomAgentWithLightPack>& agent) {
				p.put(agent->very_important_data);
			}

			static PtrWrapper<CustomAgentWithLightPack> from_datapack(const LightObjectPack& p) {
				return new CustomAgentWithLightPack(p.get<int>());
			}
		};
}}}

TEST(AgentSerializer, object_pack) {
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

// LightObjectPack with default constructible agent (no explicit
// LightSerializer specialization)
TEST(AgentSerializer, light_object_pack_default_constructible) {
	fpmas::model::AgentPtr ptr(new DefaultConstructibleCustomAgent);

	fpmas::io::datapack::ObjectPack classic_pack = ptr;
	fpmas::io::datapack::LightObjectPack light_pack = ptr;

	ASSERT_LT(light_pack.data().size, classic_pack.data().size);

	fpmas::model::AgentPtr unserialized_agent = light_pack.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<DefaultConstructibleCustomAgent*>(Not(IsNull()))
			);
}

// Light pack with non default constructible agent (no explicit light_serializer
// specialization). Falls back to fpmas::io::datapack::ObjectPack.
TEST(AgentSerializer, light_object_pack_not_default_constructible) {
	fpmas::model::AgentPtr ptr(new CustomAgent(4.2f));

	fpmas::io::datapack::LightObjectPack light_pack = ptr;
	fpmas::model::AgentPtr unserialized_agent = light_pack.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<CustomAgent*>(Not(IsNull()))
			);

	// Ensures that adl_serializer was used
	ASSERT_FLOAT_EQ(
			dynamic_cast<CustomAgent*>(unserialized_agent.get())->getData(),
			4.2f
			);
}

// User defined light_json specialization
TEST(AgentSerializer, custom_light_pack) {
	fpmas::model::AgentPtr ptr(new CustomAgentWithLightPack(12));

	fpmas::io::datapack::LightObjectPack light_pack = ptr;
	fpmas::model::AgentPtr unserialized_agent = light_pack.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<CustomAgentWithLightPack*>(Not(IsNull()))
			);

	// Ensures that adl_serializer was used
	ASSERT_FLOAT_EQ(
			dynamic_cast<CustomAgentWithLightPack*>(unserialized_agent.get())
			->very_important_data,
			12
			);
}
