#ifndef MODEL_SERIALIZER_H
#define MODEL_SERIALIZER_H

/** \file src/fpmas/model/serializer.h
 * Agent serialization related objects.
 */

#include <stdexcept>
#include "fpmas/api/model/model.h"
#include "fpmas/utils/log.h"
#include "fpmas/io/json.h"
#include "fpmas/io/datapack.h"

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
	fpmas::register_types<__VA_ARGS__ , void>();

/**
 * Defines the nlohmann specializations required to handle the JSON
 * serialization of the specified set of \Agent types.
 *
 * Notice that this is only the _definitions_ of the `to_json` and `from_json`
 * methods, that are declared (but not defined) in src/fpmas/model/model.h.
 *
 * In consequence, this macro must be invoked exaclty once from a **source**
 * file, **at the global definition level**, in any C++ target using FPMAS.
 *
 * It is possible to call the FPMAS_DEFAULT_JSON_SET_UP() macro to set up the
 * JSON serialization without specifying any Agent.
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
		void adl_serializer<fpmas::api::model::AgentPtr>\
			::to_json(json& j, const fpmas::api::model::AgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<json, __VA_ARGS__ , void>::to_json(j, data);\
		}\
		fpmas::api::model::AgentPtr adl_serializer<fpmas::api::model::AgentPtr>\
			::from_json(const json& j) {\
			return {fpmas::model::AgentPtrSerializer<json, __VA_ARGS__ , void>::from_json(j)};\
		}\
\
		void adl_serializer<fpmas::api::model::WeakAgentPtr>\
			::to_json(json& j, const fpmas::api::model::WeakAgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<json, __VA_ARGS__ , void>::to_json(j, data);\
		}\
		fpmas::api::model::WeakAgentPtr adl_serializer<fpmas::api::model::WeakAgentPtr>\
			::from_json(const json& j) {\
			return fpmas::model::AgentPtrSerializer<json, __VA_ARGS__ , void>::from_json(j);\
		}\
	}\
	namespace fpmas { namespace io { namespace json {\
		void light_serializer<fpmas::api::model::AgentPtr>::to_json(light_json& j, const fpmas::api::model::AgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<light_json, __VA_ARGS__ , void>::to_json(j, data);\
		}\
		fpmas::api::model::AgentPtr light_serializer<fpmas::api::model::AgentPtr>::from_json(const light_json& j) {\
			return {fpmas::model::AgentPtrSerializer<light_json, __VA_ARGS__ , void>::from_json(j)};\
		}\
		\
		void light_serializer<fpmas::api::model::WeakAgentPtr>\
			::to_json(light_json& j, const fpmas::api::model::WeakAgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<light_json, __VA_ARGS__ , void>::to_json(j, data);\
		}\
		fpmas::api::model::WeakAgentPtr light_serializer<fpmas::api::model::WeakAgentPtr>\
			::from_json(const light_json& j) {\
			return fpmas::model::AgentPtrSerializer<light_json, __VA_ARGS__ , void>::from_json(j);\
		}\
	}}}\

/**
 * Can be used instead of FPMAS_JSON_SET_UP() to set up json serialization
 * without specifying any agent type.
 */
#define FPMAS_DEFAULT_JSON_SET_UP()\
	namespace nlohmann {\
		void adl_serializer<fpmas::api::model::AgentPtr>\
			::to_json(json& j, const fpmas::api::model::AgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<json, void>::to_json(j, data);\
		}\
		fpmas::api::model::AgentPtr adl_serializer<fpmas::api::model::AgentPtr>\
			::from_json(const json& j) {\
			return {fpmas::model::AgentPtrSerializer<json, void>::from_json(j)};\
		}\
\
		void adl_serializer<fpmas::api::model::WeakAgentPtr>\
			::to_json(json& j, const fpmas::api::model::WeakAgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<json, void>::to_json(j, data);\
		}\
		fpmas::api::model::WeakAgentPtr adl_serializer<fpmas::api::model::WeakAgentPtr>\
			::from_json(const json& j) {\
			return fpmas::model::AgentPtrSerializer<json, void>::from_json(j);\
		}\
	}\
	namespace fpmas { namespace io { namespace json {\
		void light_serializer<fpmas::api::model::AgentPtr>::to_json(light_json& j, const fpmas::api::model::AgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<light_json, void>::to_json(j, data);\
		}\
		fpmas::api::model::AgentPtr light_serializer<fpmas::api::model::AgentPtr>::from_json(const light_json& j) {\
			return {fpmas::model::AgentPtrSerializer<light_json, void>::from_json(j)};\
		}\
		\
		void light_serializer<fpmas::api::model::WeakAgentPtr>\
			::to_json(light_json& j, const fpmas::api::model::WeakAgentPtr& data) {\
			fpmas::model::AgentPtrSerializer<light_json, void>::to_json(j, data);\
		}\
		fpmas::api::model::WeakAgentPtr light_serializer<fpmas::api::model::WeakAgentPtr>\
			::from_json(const light_json& j) {\
			return fpmas::model::AgentPtrSerializer<light_json, void>::from_json(j);\
		}\
	}}}\

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
				template<typename JsonType>\
				static void to_json(JsonType& j, const fpmas::api::utils::PtrWrapper<AGENT>&) {\
					j = json::object();\
				}\
\
				/**\
				 * Unserializes a default AGENT, dynamically allocated with `new AGENT`.
				 *
				 * @param ptr unserialized AGENT pointer
				 */\
				template<typename JsonType>\
				static void from_json(const JsonType&, fpmas::api::utils::PtrWrapper<AGENT>& ptr) {\
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
			template<typename JsonType>
			static std::type_index from_json(const JsonType& j) {
				std::size_t id = j.template get<std::size_t>();
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
			template<typename JsonType>
			static void to_json(JsonType& j, const std::type_index& type) {
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
			template<typename JsonType>
			static fpmas::api::utils::PtrWrapper<AgentType> from_json(const JsonType& j) {
				return AgentType::from_json(j);
			}

			/**
			 * Calls `AgentType::to_json(j, agent_ptr.get())`.
			 *
			 * @param j json
			 * @param agent_ptr agent pointer to serialized
			 */
			template<typename JsonType>
			static void to_json(JsonType& j, const fpmas::api::utils::PtrWrapper<AgentType>& agent_ptr) {
				AgentType::to_json(j, agent_ptr.get());
			}
		};
}

namespace fpmas { namespace io { namespace json {

#define AGENT_TYPE_STR(AgentType) typeid(AgentType).name()

	/**
	 * \anchor not_default_constructible_Agent_light_serializer
	 *
	 * A default light_serializer specialization for \Agent types, when no
	 * default constructor is available. In this case, to_json() and
	 * from_json() methods falls back to the classic `nlohmann::json`
	 * serialization rules, what might be inefficient. A warning is also
	 * printed at runtime.
	 *
	 * To avoid this inefficient behaviors, two things can be performed:
	 * - Defining a default constructor for `AgentType`. Notice that the
	 *   default constructed `AgentType` will **never** be used by FPMAS (since
	 *   `AgentType` transmitted in a \light_json are completely passive,
	 *   a complete \DistributedNode transfer is always performed before
	 *   when the \Agent data is required).
	 * - Specializing the light_serializer with `AgentType`:
	 *   ```cpp
	 *   namespace fpmas { namespace io { namespace json {
	 *   	template<>
	 *   		struct light_serializer<fpmas::api::utils::PtrWrapper<CustomAgentType>> {
	 *   			static void to_json(light_json& j, const fpmas::api::utils::PtrWrapper<CustomAgentType>& ptr) {
	 *   				// light_json serialization rules
	 *   				...
	 *   			}
	 *
	 *   			static fpmas::api::utils::PtrWrapper<CustomAgentType> to_json(light_json& j) {
	 *   				// light_json unserialization rules
	 *   				...
	 *   			}
	 *   		};
	 *   }}}
	 *   ```
	 *
	 * @tparam AgentType concrete type of \Agent to serialize
	 */
	template<typename AgentType>
		struct light_serializer<
		fpmas::api::utils::PtrWrapper<AgentType>,
		typename std::enable_if<
			std::is_base_of<fpmas::api::model::Agent, AgentType>::value
			&& !std::is_default_constructible<AgentType>::value
			&& std::is_same<AgentType, typename AgentType::FinalAgentType>::value>::type
			>{
				public:
					/**
					 * \anchor not_default_constructible_Agent_light_serializer_to_json
					 *
					 * Calls
					 * `nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<AgentType>>::%to_json()`.
					 *
					 * @param j json output
					 * @param agent agent to serialize
					 */
					static void to_json(light_json& j, const fpmas::api::utils::PtrWrapper<AgentType>& agent) {
						static bool warn_print = false;
						if(!warn_print) {
							warn_print = true;
							FPMAS_LOGW(0, "light_serializer",
									"Type %s does not define a default constructor or "
									"a light_serializer specialization."
									"This might be uneficient.", AGENT_TYPE_STR(AgentType)
									);
						}

						nlohmann::json _j = agent;
						j = light_json::parse(_j.dump());
					}

					/**
					 * \anchor not_default_constructible_Agent_light_serializer_from_json
					 *
					 * Calls
					 * `nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<AgentType>>::%from_json()`.
					 *
					 * @param j json input
					 * @return dynamically allocated `AgentType`
					 */
					static fpmas::api::utils::PtrWrapper<AgentType> from_json(const light_json& j) {
						nlohmann::json _j = nlohmann::json::parse(j.dump());
						return _j.get<fpmas::api::utils::PtrWrapper<AgentType>>();
					}
			};

	
	/**
	 * \anchor default_constructible_Agent_light_serializer
	 *
	 * A default light_serializer specialization for default constructible
	 * \Agent types.
	 *
	 * When a default constructor is available for `AgentType`, this
	 * specialization is automatically selected.
	 *
	 * The usage of this specialization is recommended, since its optimal:
	 * **no** json data is required to serialize / unserialize a \light_json
	 * representing such an `AgentType`.
	 *
	 * @tparam AgentType concrete type of \Agent to serialize
	 */
	template<typename AgentType>
		struct light_serializer<
			fpmas::api::utils::PtrWrapper<AgentType>,
		 	typename std::enable_if<
				std::is_base_of<fpmas::api::model::Agent, AgentType>::value
				&& std::is_default_constructible<AgentType>::value
				&& std::is_same<AgentType, typename AgentType::FinalAgentType>::value>::type
		 > {

			/**
			 * \anchor default_constructible_Agent_light_serializer_to_json
			 *
			 * \light_json to_json() implementation for the default
			 * constructible `AgentType`: nothing is serialized.  Notice that
			 * the "type id", that is actually required to reach this method,
			 * is managed by the
			 * nlohmann::adl_serializer<fpmas::api::model::AgentPtr> structure.
			 */
			static void to_json(light_json&, const fpmas::api::utils::PtrWrapper<AgentType>&) {
			}

			/**
			 * \anchor default_constructible_Agent_light_serializer_from_json
			 *
			 * \light_json from_json() implementation for the default
			 * constructible `AgentType`: a dynamically allocated, default
			 * constructed `AgentType` is returned.
			 *
			 * @return dynamically allocated `AgentType`
			 */
			static fpmas::api::utils::PtrWrapper<AgentType> from_json(const light_json&) {
				return new AgentType;
			}
		};

}}}

namespace fpmas { namespace io { namespace datapack {
	template<>
		struct Serializer<api::model::AgentPtr>
		: public JsonSerializer<api::model::AgentPtr> {
		};

	template<>
		struct LightSerializer<api::model::AgentPtr>
		: public LightJsonSerializer<api::model::AgentPtr> {
		};

}}}

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
		using api::model::WeakAgentPtr;

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

		template<typename JsonType, typename Type, typename... AgentTypes> 
			struct AgentPtrSerializer;

		/**
		 * AgentPtrSerializer recursion base case.
		 */
		template<typename JsonType> 
			struct AgentPtrSerializer<JsonType, void> {
				/**
				 * to_json recursion base case.
				 *
				 * Reaching this case is erroneous and throws a exceptions::BadTypeException.
				 *
				 * @throw exceptions::BadTypeException
				 */
				static void to_json(JsonType&, const WeakAgentPtr& ptr); 

				/**
				 * from_json recursion base case.
				 *
				 * Reaching this case is erroneous and throws a exceptions::BadIdException.
				 *
				 * @throw exceptions::BadIdException
				 */
				static WeakAgentPtr from_json(const JsonType& j);
			};

		template<typename JsonType>
			void AgentPtrSerializer<JsonType, void>::to_json(
					JsonType &, const WeakAgentPtr& ptr) {
				FPMAS_LOGE(-1, "AGENT_SERIALIZER",
						"Invalid agent type : %s. Make sure to properly register "
						"the Agent type with FPMAS_JSON_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
						ptr->typeId().name());
				throw exceptions::BadTypeException(ptr->typeId());
			}

		template<typename JsonType>
			WeakAgentPtr AgentPtrSerializer<JsonType, void>::from_json(const JsonType &j) {
				std::size_t id = j.at("type").template get<std::size_t>();
				FPMAS_LOGE(-1, "AGENT_SERIALIZER",
						"Invalid agent type id : %lu. Make sure to properly register "
						"the Agent type with FPMAS_JSON_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
						id);
				throw exceptions::BadIdException(id);
			}

		/**
		 * Generic AgentPtr serializer, that recursively deduces the final and
		 * concrete types of polymorphic AgentPtr instances.
		 *
		 * @tparam JsonType type of json used for serialization
		 * @tparam Type type currently inspected
		 * @tparam AgentTypes agent types not inspected yet (must end with
		 * `void`)
		 */
		template<typename JsonType, typename Type, typename... AgentTypes> 
			struct AgentPtrSerializer {
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
				 * @param j json
				 * @param ptr polymorphic agent pointer to serialize
				 *
				 * @see nlohmann::adl_serializer<std::type_index>::to_json (used to
				 * serialize the agent type id)
				 */
				static void to_json(
						JsonType& j, const WeakAgentPtr& ptr) {
					if(ptr->typeId() == Type::TYPE_ID) {
						j["type"] = Type::TYPE_ID;
						j["gids"] = ptr->groupIds();
						j["agent"] = TypedAgentPtr<Type>(
								const_cast<Type*>(dynamic_cast<const Type*>(ptr.get()))
								);
					} else {
						AgentPtrSerializer<JsonType, AgentTypes...>::to_json(j, ptr);
					}
				}

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
				 * @param j json
				 * @return ptr unserialized polymorphic agent pointer
				 *
				 * @see nlohmann::adl_serializer<std::type_index>::from_json (used to
				 * unserialize the agent type id)
				 */
				static WeakAgentPtr from_json(const JsonType& j) {
					fpmas::api::model::TypeId id = j.at("type").template get<fpmas::api::model::TypeId>();
					if(id == Type::TYPE_ID) {
						auto agent = j.at("agent").template get<TypedAgentPtr<Type>>();
						for(auto gid : j.at("gids")
								.template get<std::vector<fpmas::api::model::GroupId>>())
							agent->addGroupId(gid);
						return {agent};
					} else {
						return AgentPtrSerializer<JsonType, AgentTypes...>::from_json(j);
					}
				}

			};
	}
}
#endif
