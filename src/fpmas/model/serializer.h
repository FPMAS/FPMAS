#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

/** \file src/fpmas/model/serializer.h
 * Agent serialization related objects.
 */

#include <stdexcept>
#include "fpmas/api/model/model.h"
#include "fpmas/utils/log.h"

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
	fpmas::register_types<__VA_ARGS__, void>();

/**
 * Defines the nlohmann specializations required to handle the JSON
 * serialization of the specified set of \Agent types.
 *
 * Notice that this is only the _definitions_ of the `to_json` and `from_json`
 * methods, that are declared (but not defined) in src/fpmas/model/model.h.
 *
 * In consequence, this macro must be invoked exaclty once from a **source**
 * file in any C++ target using FPMAS.
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
#define FPMAS_JSON_SET_UP(...)\
	namespace nlohmann {\
		void adl_serializer<fpmas::api::model::AgentPtr>::to_json(json& j, const fpmas::api::model::AgentPtr& data) {\
			fpmas::model::to_json<__VA_ARGS__, void>(j, data);\
		}\
		fpmas::api::model::AgentPtr adl_serializer<fpmas::api::model::AgentPtr>::from_json(const json& j) {\
			return std::move(fpmas::model::from_json<__VA_ARGS__, void>(j));\
		}\
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
			/**\
			 * Default AGENT adl_serializer specialization.
			 */\
			struct adl_serializer<fpmas::api::utils::PtrWrapper<AGENT>> {\
				/**\
				 * No effect: j is assigned a void object.
				 *
				 * @param j output json
				 */\
				static void to_json(json& j, const fpmas::api::utils::PtrWrapper<AGENT>&) {\
					j = json::object();\
				}\
\
				/**\
				 * Unserializes a default AGENT, dynamically allocated with `new AGENT`.
				 *
				 * @param ptr unserialized AGENT pointer
				 */\
				static void from_json(const json&, fpmas::api::utils::PtrWrapper<AGENT>& ptr) {\
					ptr = fpmas::api::utils::PtrWrapper<AGENT>(new AGENT);\
				}\
			};\
	}\

namespace fpmas { namespace exceptions {
	/**
	 * Exception raised when unserializing an std::type_index if the provided
	 * type id does not correspond to any registered type.
	 *
	 * @see register_types
	 * @see nlohmann::adl_serializer<std::type_index>
	 */
	class BadIdException : public std::exception {
		private:
			std::string message;

		public:
			/**
			 * BadIdException constructor.
			 *
			 * @param bad_id unregistered id
			 */
			BadIdException(std::size_t bad_id)
				: message("Unknown type id : " + std::to_string(bad_id)) {}

			/**
			 * Returns the explanatory string.
			 *
			 * @see https://en.cppreference.com/w/cpp/error/exception/what
			 */
			const char* what() const noexcept override {
				return message.c_str();
			}
	};

	/**
	 * Exception raised when serializing an std::type_index if the provided
	 * type index not correspond to any registered type.
	 *
	 * @see register_types
	 * @see nlohmann::adl_serializer<std::type_index>
	 */
	class BadTypeException : public std::exception {
		private:
			std::string message;

		public:
			/**
			 * BadTypeException constructor.
			 *
			 * @param bad_type unregistered std::type_index
			 */
			BadTypeException(const std::type_index& bad_type)
				: message("Unknown type index : " + std::string(bad_type.name())) {}

			/**
			 * Returns the explanatory string.
			 *
			 * @see https://en.cppreference.com/w/cpp/error/exception/what
			 */
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

	/**
	 * Generic \Agent pointer serialization.
	 *
	 * This partial specialization can be used to easily define serialization
	 * rules directly from the \Agent implementation, using the following
	 * static method definitions :
	 * ```cpp
	 * class UserDefinedAgent : fpmas::model::AgentBase<UserDefinedAgent> {
	 * 	...
	 * 	public:
	 * 		...
	 * 		static void to_json(
	 * 			nlohmann::json& j,
	 * 			const UserDefinedAgent* agent
	 * 			);
	 * 		static UserDefinedAgent* from_json(
	 * 			const nlohmann::json& j
	 * 			);
	 * 		...
	 * 	};
	 * 	```
	 * The from_json method is assumed to return an **heap allocated** agent
	 * (initialized with a `new` statement).
	 *
	 * \par Example
	 * ```cpp
	 * class Agent1 : public fpmas::model::AgentBase<Agent1> {
	 * 	private:
	 * 		int count;
	 * 		std::string message;
	 * 	public:
	 * 		Agent1(int count, std::string message)
	 * 			: count(count), message(message) {}
	 *
	 * 		static void to_json(nlohmann::json& j, const Agent1* agent) {
	 * 			j["c"] = agent->count;
	 * 			j["m"] = agent->message;
	 * 		}
	 *
	 * 		static Agent1* from_json(const nlohmann::json& j) {
	 * 			return new Agent1(
	 * 				j.at("c").get<int>(),
	 * 				j.at("m").get<std::string>()
	 * 				);
	 * 		}
	 *
	 * 		void act() override;
	 * 	};
	 * ```
	 *
	 * \note
	 * Notice that its still possible to define adl_serializer specialization
	 * without using the internal to_json / from_json methods.
	 *
	 * \par Example
	 * ```cpp
	 * class Agent1 : public fpmas::model::AgentBase<Agent1> {
	 * 	private:
	 * 		int count;
	 * 		std::string message;
	 * 	public:
	 * 		Agent1(int count, std::string message)
	 * 			: count(count), message(message) {}
	 *
	 * 		int getCount() const {return count;}
	 * 		std::string getMessage() const {return message;}
	 *
	 * 		void act() override;
	 * 	};
	 *
	 * 	namespace nlohmann {
	 * 		template<>
	 * 			struct adl_serializer<fpmas::api::utils::PtrWrapper<Agent1>> {
	 * 				static void to_json(
	 * 					json& j,
	 * 					fpmas::api::utils::PtrWrapper<Agent1>& agent_ptr
	 * 				) {
	 * 					j["c"] = agent_ptr->getCount();
	 * 					j["m"] = agent_ptr->getMessage();
	 * 				}
	 * 				
	 * 				static fpmas::api::utils::PtrWrapper<Agent1> from_json(
	 * 					json& j
	 * 				) {
	 * 					return {new Agent1(
	 * 						j.at("c").get<int>(),
	 * 						j.at("m").get<std::string>()
	 * 						)};
	 * 				}
	 * 		};
	 * 	}
	 * 	```
	 */
	template<typename AgentType>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<AgentType>> {
			/**
			 * Returns a PtrWrapper initialized with `AgentType::from_json(j)`.
			 *
			 * @param j json
			 * @return unserialized agent pointer
			 */
			static fpmas::api::utils::PtrWrapper<AgentType> from_json(const json& j) {
				return AgentType::from_json(j);
			}

			/**
			 * Calls `AgentType::to_json(j, agent_ptr.get())`.
			 *
			 * @param j json
			 * @param agent_ptr agent pointer to serialized
			 */
			static void to_json(json& j, const fpmas::api::utils::PtrWrapper<AgentType>& agent_ptr) {
				AgentType::to_json(j, agent_ptr.get());
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

		/**
		 * Typed \Agent pointer.
		 *
		 * Contrary to api::model::AgentPtr, that represents a polymorphic
		 * \Agent (i.e. api::utils::PtrWrapper<api::model::Agent>), this
		 * pointer is used to wrap a concrete \Agent type (e.g.
		 * api::utils::PtrWrapper<UserAgent1>).
		 *
		 * This type is notably used to define json serialization rules for
		 * user defined \Agent types.
		 *
		 * @tparam Agent concrete agent type
		 */
		template<typename Agent>
			using TypedAgentPtr = api::utils::PtrWrapper<Agent>;

		template<typename Type, typename... AgentTypes> 
			void to_json(nlohmann::json& j, const AgentPtr& ptr);

		/**
		 * to_json recursion base case.
		 *
		 * Reaching this case is erroneous and throws a exceptions::BadTypeException.
		 *
		 * @throw exceptions::BadTypeException
		 */
		template<> 
			void to_json<void>(nlohmann::json& j, const AgentPtr& ptr);


		/**
		 * Recursive to_json method used to serialize polymorphic \Agent
		 * pointers as JSON.
		 *
		 * `Type` corresponds to the currently examined \Agent type. If this
		 * type id (i.e. Type::TYPE_ID) corresponds to the `ptr` \Agent type
		 * (i.e.  `ptr->typedId()`), the underlying agent is serialized
		 * using the user provided
		 * nlohmann::adl_serializer<TypedAgentPtr<Type>>.
		 *
		 * Else, attempts to recursively serialize `ptr` with
		 * `to_json<AgentTypes...>(j, ptr)`. Notice that the last value of
		 * `AgentTypes` **must** be void to reach the recursion base case (see
		 * to_json<void>).
		 *
		 * @tparam Type currently examined type
		 * @tparam AgentTypes agent types not examined yet (must end with
		 * `void`)
		 * @param j json
		 * @param ptr polymorphic agent pointer to serialize
		 *
		 * @see nlohmann::adl_serializer<std::type_index>::to_json (used to
		 * serialize the agent type id)
		 */
		template<typename Type, typename... AgentTypes> 
			void to_json(nlohmann::json& j, const AgentPtr& ptr) {
				if(ptr->typeId() == Type::TYPE_ID) {
					j["type"] = Type::TYPE_ID;
					j["gids"] = ptr->groupIds();
					j["agent"] = TypedAgentPtr<Type>(const_cast<Type*>(dynamic_cast<const Type*>(ptr.get())));
				} else {
					to_json<AgentTypes...>(j, ptr);
				}
			}

		template<typename Type, typename... AgentTypes> 
			AgentPtr from_json(const nlohmann::json& j);

		/**
		 * from_json recursion base case.
		 *
		 * Reaching this case is erroneous and throws a exceptions::BadIdException.
		 *
		 * @throw exceptions::BadIdException
		 */
		template<> 
			AgentPtr from_json<void>(const nlohmann::json& j);

		/**
		 * Recursive from_json method used to unserialize polymorphic \Agent
		 * pointers from JSON.
		 *
		 * `Type` corresponds to the currently examined \Agent type. If this
		 * type id (i.e. Type::TYPE_ID) corresponds to the JSON type id value
		 * (i.e. j.at("type").get<fpmas::api::model::TypeId>()), the JSON value
		 * is unserialized using the user provided
		 * nlohmann::adl_serializer<TypedAgentPtr<Type>>.
		 *
		 * Else, attempts to recursively unserialize the JSON value with
		 * `from_json<AgentTypes...>(j, ptr)`. Notice that the last value of
		 * `AgentTypes` **must** be void to reach the recursion base case (see
		 * from_json<void>).
		 *
		 * @tparam Type currently examined type
		 * @tparam AgentTypes agent types not examined yet (must end with
		 * `void`)
		 * @param j json
		 * @return ptr unserialized polymorphic agent pointer
		 *
		 * @see nlohmann::adl_serializer<std::type_index>::from_json (used to
		 * unserialize the agent type id)
		 */
		template<typename Type, typename... AgentTypes> 
			AgentPtr from_json(const nlohmann::json& j) {
				fpmas::api::model::TypeId id = j.at("type").get<fpmas::api::model::TypeId>();
				if(id == Type::TYPE_ID) {
					auto agent = j.at("agent").get<TypedAgentPtr<Type>>();
					for(auto gid : j.at("gids")
							.get<std::vector<fpmas::api::model::GroupId>>())
						agent->addGroupId(gid);
					return {agent};
				} else {
					return std::move(from_json<AgentTypes...>(j));
				}
			}
	}
}
#endif
