#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

#include <stdexcept>
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/api/model/model.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace model {
	using api::model::AgentPtr;
	template<typename T>
		using TypedAgentPtr = api::utils::VirtualPtrWrapper<T>;

	template<typename Type, typename... AgentTypes> 
		void to_json(nlohmann::json& j, const AgentPtr& ptr);

	template<> 
		void to_json<void>(nlohmann::json& j, const AgentPtr& ptr) {
			FPMAS_LOGE(-1, "AGENT_SERIALIZER",
					"Invalid agent type : %lu. Make sure to properly register \
						the Agent type with FPMAS_JSON_SERIALIZE_AGENT.",
						ptr->typeId());
			std::string message = "Invalid agent type : " + std::to_string(ptr->typeId());
			throw std::invalid_argument(message);
		}

	template<typename Type, typename... AgentTypes> 
		void to_json(nlohmann::json& j, const AgentPtr& ptr) {
			if(ptr->typeId() == Type::TYPE_ID) {
				j["type"] = Type::TYPE_ID;
				j["gid"] = ptr->groupId();
				j["agent"] = TypedAgentPtr<Type>(const_cast<Type*>(static_cast<const Type*>(ptr.get())));
			} else {
				to_json<AgentTypes...>(j, ptr);
			}
		}

	template<typename Type, typename... AgentTypes> 
		AgentPtr from_json(const nlohmann::json& j);

	template<> 
		AgentPtr from_json<void>(const nlohmann::json& j) {
			fpmas::api::model::TypeId id = j.at("type").get<fpmas::api::model::TypeId>();
			FPMAS_LOGE(-1, "AGENT_SERIALIZER", "Invalid agent type : %lu", id);
			std::string message = "Invalid agent type : " + std::to_string(id);
			throw std::invalid_argument(message);
		}

	template<typename Type, typename... AgentTypes> 
		AgentPtr from_json(const nlohmann::json& j) {
			fpmas::api::model::TypeId id = j.at("type").get<fpmas::api::model::TypeId>();
			if(id == Type::TYPE_ID) {
				auto agent = j.at("agent").get<TypedAgentPtr<Type>>();
				agent->setGroupId(j.at("gid").get<fpmas::api::model::GroupId>());
				return {agent};
			} else {
				return std::move(from_json<AgentTypes...>(j));
			}
		}
}}

#define FPMAS_JSON_SERIALIZE_AGENT(...) \
	namespace nlohmann {\
		using fpmas::api::model::AgentPtr;\
		template<>\
		struct adl_serializer<AgentPtr> {\
			static void to_json(json& j, const AgentPtr& data) {\
				fpmas::model::to_json<__VA_ARGS__, void>(j, data);\
			}\
			static AgentPtr from_json(const json& j) {\
				return std::move(fpmas::model::from_json<__VA_ARGS__, void>(j));\
			}\
		};\
	}\

#endif
