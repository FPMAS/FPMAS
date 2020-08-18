#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

/** \file src/fpmas/model/serializer.h
 * Agent serialization related objects.
 */

#include <stdexcept>
#include "fpmas/api/model/model.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace exceptions {
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

	class BadTypeException : public std::exception {
		private:
			std::string message;

		public:
			BadTypeException(const std::type_index& bad_type)
				: message("Unknown type id : " + std::string(bad_type.name())) {}

			const char* what() const noexcept override {
				return message.c_str();
			}
	};
}}

namespace nlohmann {
	
	/**
	 * An nlohmann::adl_serializer specialization to allow JSON serialization of
	 * [std:type_index](https://en.cppreference.com/w/cpp/types/type_index),
	 * that is used as \Agents TypeId.
	 *
	 * Each registered std::type_index is automatically mapped to a unique
	 * std::size_t integer to allow serialization.
	 */
	template<>
		struct adl_serializer<std::type_index> {
			private:
			static std::size_t id;
			static std::unordered_map<std::size_t, std::type_index> id_to_type;
			static std::unordered_map<std::type_index, std::size_t> type_to_id;

			public:
			/**
			 * Register an std::type_index instance so that it can be
			 * serialized.
			 *
			 * A unique integer id is internally associated to the type and
			 * used for serialization.
			 *
			 * The fpmas::register_types() function template and the
			 * FPMAS_REGISTER_AGENT_TYPES(...) macro can be used to easily register
			 * types.
			 *
			 * @param type type to register
			 */
			static std::size_t register_type(const std::type_index& type) {
				std::size_t new_id = id++;
				type_to_id.insert({type, new_id});
				id_to_type.insert({new_id, type});
				return new_id;
			}

			/**
			 * Unserializes an std::type_index instance from the specified JSON.
			 *
			 * @param j json to unserialize
			 * @throw fpmas::exceptions::BadIdException if the id does not correspond to a type
			 * registered using register_type()
			 */
			static std::type_index from_json(const json& j) {
				std::size_t id = j.get<std::size_t>();
				auto _type = id_to_type.find(id);
				if(_type != id_to_type.end())
					return _type->second;
				throw fpmas::exceptions::BadIdException(id);

			}

			/**
			 * Serializes an std::type_index instance into the specified JSON.
			 *
			 * @param j json instance
			 * @param type type to serialize
			 * @throw fpmas::exceptions::BadTypeException if the type was not
			 * register using register_type()
			 */
			static void to_json(json& j, const std::type_index& type) {
				auto _id = type_to_id.find(type);
				if(_id != type_to_id.end())
					j = _id->second;
				else
					throw fpmas::exceptions::BadTypeException(type);
			}
		};
}

namespace fpmas { 
	template<typename _T, typename... T>
		void register_types();

	/**
	 * Register_types recursion base case.
	 */
	template<> void register_types<void>();

	/**
	 * Recursive function template used to register a set of types into the
	 * adl_serializer<std::type_index>, so that their corresponding
	 * std::type_index can be serialized.
	 *
	 * The last type specified **MUST** be void for the recursion to be valid.
	 *
	 * @par Example
	 * ```cpp
	 * nlohmann::fpmas::register_types<Type1, Type2, void>();
	 * ```
	 *
	 * The FPMAS_REGISTER_AGENT_TYPES(...) macro can be used instead as a more expressive
	 * and user-friendly way to register types.
	 */
	template<typename _T, typename... T>
		void register_types() {
			nlohmann::adl_serializer<std::type_index>::register_type(typeid(_T));
			register_types<T...>();
		}

	namespace model {
		using api::model::AgentPtr;
		template<typename T>
			using TypedAgentPtr = api::utils::VirtualPtrWrapper<T>;

		template<typename Type, typename... AgentTypes> 
			void to_json(nlohmann::json& j, const AgentPtr& ptr);

		template<> 
			void to_json<void>(nlohmann::json& j, const AgentPtr& ptr);

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
			AgentPtr from_json<void>(const nlohmann::json& j);

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

/**
 * Registers the specified \Agent types so that they can be serialized as JSON
 * using the `nlohmann` library.
 *
 * More particularly, registers \Agents types so that their TypeId (i.e. their
 * corresponding std::type_index) can be serialized. Serialization rules
 * (to_json and from_json methods) for each type must still be specified.
 *
 * This macro must be called **at runtime**, as soon as possible, before
 * any \Agent serialization occurs.
 *
 * @par Example
 * ```cpp
 * ...
 *
 * int main(int argc, char** argv) {
 * 	fpmas::init(argc, argv);
 *
 * 	FPMAS_REGISTER_AGENT_TYPES(Agent1, Agent2, OtherAgent);
 *
 * 	...
 *
 * 	fpmas::finalize();
 * }
 * ```
 */
#define FPMAS_REGISTER_AGENT_TYPES(...)\
	fpmas::register_types<__VA_ARGS__, void>();\

/**
 * Defines the nlohmann specializations required to handle the JSON
 * serialization of the specified set of \Agent types.
 *
 * This macro must be called **at the global definition level**.
 *
 * The same set of \Agent types must be registered at runtime using the
 * FPMAS_REGISTER_AGENT_TYPES(...) macro at runtime.
 *
 * @par Example
 * ```cpp
 * #define AGENT_TYPES Agent1, Agent2, OtherAgent
 *
 * FPMAS_JSON_SET_UP(AGENT_TYPES)
 *
 * int main(int argc, char** argv) {
 * 	fpmas::init(argc, argv);
 *
 * 	FPMAS_REGISTER_AGENT_TYPES(AGENT_TYPES);
 *
 * 	...
 *
 * 	fpmas::finalize();
 * }
 * ```
 */
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

/**
 * Helper macro to easily define JSON serialization rules for
 * [DefaultConstructible](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible)
 * \Agents.
 *
 * @param AGENT DefaultConstructible \Agent type
 */
#define FPMAS_DEFAULT_JSON(AGENT) \
	namespace nlohmann {\
		template<>\
			struct adl_serializer<fpmas::api::utils::VirtualPtrWrapper<AGENT>> {\
				static void to_json(json& j, const fpmas::api::utils::VirtualPtrWrapper<AGENT>& data) {\
				}\
\
				static void from_json(const json& j, fpmas::api::utils::VirtualPtrWrapper<AGENT>& ptr) {\
					ptr = fpmas::api::utils::VirtualPtrWrapper<AGENT>(new AGENT);\
				}\
			};\
	}\

#endif
