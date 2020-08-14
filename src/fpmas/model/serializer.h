#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

#include <stdexcept>
#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/api/model/model.h"
#include "fpmas/utils/log.h"

namespace nlohmann {
	class BadIdException : public std::exception {
		private:
			std::string message;

		public:
			BadIdException(std::size_t bad_id)
				: message("Unknown type id : " + std::to_string(bad_id)) {}

			const char* what() const noexcept override {
				return message.c_str();
			}
	};

	template<>
		struct adl_serializer<std::type_index> {
			static std::size_t id;
			static std::unordered_map<std::size_t, std::type_index> id_to_type;
			static std::unordered_map<std::type_index, std::size_t> type_to_id;

			static std::size_t register_type(const std::type_index& type) {
				std::size_t new_id = id++;
				type_to_id.insert({type, new_id});
				id_to_type.insert({new_id, type});
				return new_id;
			}

			static std::type_index from_json(const json& j) {
				std::size_t id = j.get<std::size_t>();
				auto _type = id_to_type.find(id);
				if(_type != id_to_type.end())
					return _type->second;
				throw BadIdException(id);

			}

			static void to_json(json& j, const std::type_index& type) {
				auto _id = type_to_id.find(type);
				if(_id == type_to_id.end()) {
					j = register_type(type);
				} else {
					j = _id->second;
				}
			}
		};
	std::unordered_map<std::size_t, std::type_index> adl_serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, std::size_t> adl_serializer<std::type_index>::type_to_id;
	std::size_t adl_serializer<std::type_index>::id = 0;

	template<typename T>
		std::size_t register_type() {
			return adl_serializer<std::type_index>::register_type(typeid(T));
		}
	template<typename _T, typename... T>
		void register_types();
	template<>
		void register_types<void>() {
		}
	template<typename _T, typename... T>
		void register_types() {
			register_type<_T>();
			register_types<T...>();
		}

}

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
			std::string message = "Invalid agent type : " + std::string(ptr->typeId().name());
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
			std::string message = "Invalid agent type : " + std::string(id.name());
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

#define FPMAS_REGISTER_AGENT_TYPES(...)\
	nlohmann::register_types<__VA_ARGS__, void>();\

#define FPMAS_JSON_SET_UP(...) \
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
