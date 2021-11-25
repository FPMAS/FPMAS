#ifndef FPMAS_MODEL_SERIALIZER_H
#define FPMAS_MODEL_SERIALIZER_H

#include "json_serializer.h"
#include "datapack_serializer.h"
#include "fpmas/io/json_datapack.h"

/**
 * Defines a json based implementation of AgentPtr
 * fpmas::io::datapack::Serializer and fpmas::io::datapack::LightSerializer
 * implementation, that can be used instead of definitions provided by
 * FPMAS_BASE_DATAPACK_SET_UP().
 *
 * This allows to use the object pack serialization for DistributedNodes for
 * example, even if ObjectPack serialization rules are not provided for
 * AgentPtr (i.e.  FPMAS_BASE_DATAPACK_SET_UP() is not used).
 */
#define FPMAS_AGENT_PTR_DATAPACK_JSON_FALLBACK()\
	namespace fpmas { namespace io { namespace datapack {\
		void Serializer<AgentPtr>::to_datapack(ObjectPack& p, const AgentPtr& ptr) {\
			JsonSerializer<AgentPtr>::to_datapack(p, ptr);\
		}\
		AgentPtr Serializer<AgentPtr>::from_datapack(const ObjectPack& p) {\
			return JsonSerializer<AgentPtr>::from_datapack(p);\
		}\
		void LightSerializer<AgentPtr>::to_datapack(LightObjectPack& p, const AgentPtr& ptr) {\
			LightJsonSerializer<AgentPtr>::to_datapack(p, ptr);\
		}\
		AgentPtr LightSerializer<AgentPtr>::from_datapack(const LightObjectPack& p) {\
			return LightJsonSerializer<AgentPtr>::from_datapack(p);\
		}\
	}}}

/**
 * Defines an object pack based implementation of AgentPtr
 * nlohmann::adl_serializer and fpmas::io::json::light_serializer
 * implementation, that can be used instead of definitions provided by
 * FPMAS_BASE_JSON_SET_UP().
 *
 * This allows to use the json serialization for DistributedNodes for example,
 * even if JSON serialization rules are not provided for AgentPtr (i.e.
 * FPMAS_BASE_JSON_SET_UP() is not used).
 */
#define FPMAS_AGENT_PTR_JSON_DATAPACK_FALLBACK()\
	namespace nlohmann {\
		void adl_serializer<fpmas::api::model::AgentPtr>\
		::to_json(json& j, const fpmas::api::model::AgentPtr& ptr) {\
			fpmas::io::json::datapack_serializer<fpmas::api::model::AgentPtr>\
			::to_json(j, ptr);\
		}\
		fpmas::api::model::AgentPtr adl_serializer<fpmas::api::model::AgentPtr>\
		::from_json(const json& j) {\
			return fpmas::io::json::datapack_serializer<fpmas::api::model::AgentPtr>\
			::from_json(j);\
		}\
	}\
	namespace fpmas { namespace io { namespace json {\
		void light_serializer<api::model::AgentPtr>\
		::to_json(light_json& j, const api::model::AgentPtr& ptr) {\
			fpmas::io::json::light_datapack_serializer<api::model::AgentPtr>\
			::to_json(j, ptr);\
		}\
		api::model::AgentPtr light_serializer<api::model::AgentPtr>\
		::from_json(const light_json& j) {\
			return fpmas::io::json::light_datapack_serializer<api::model::AgentPtr>\
			::from_json(j);\
		}\
	}}}

/**
 * Sets up json based Agent serialization rules.
 *
 * This macro must be invoked exactly once from a **source** file and should
 * not be called together with FPMAS_DATAPACK_SET_UP().
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
 * 	FPMAS_REGISTER_AGENT_TYPES(AGENT_TYPES);
 *
 * 	fpmas::init(argc, argv);
 *
 * 	...
 *
 * 	fpmas::finalize();
 * }
 * ```
 *
 * ObjectPack Agent serialization is set up so that Agent are serialized into
 * JSON, stored in an ObjectPack field.
 * To prevent this behavior, FPMAS_BASE_JSON_SET_UP() and
 * FPMAS_BASE_DATAPACK_SET_UP() can be used together to enable both Agent
 * serialization techniques.
 *
 * Notice that all of this only affect **AgentPtr serialization**:
 * serialization of any other object, with JSON or ObjectPack, is not altered
 * by those macros.
 *
 * See the nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<AgentType>>
 * documentation to learn how to enable Agent datapack serialization.
 */
#define FPMAS_JSON_SET_UP(...)\
	FPMAS_BASE_JSON_SET_UP(__VA_ARGS__)\
	FPMAS_AGENT_PTR_DATAPACK_JSON_FALLBACK()

/**
 * Can be used instead of FPMAS_JSON_SET_UP() to set up json serialization
 * without specifying any Agent type.
 */
#define FPMAS_DEFAULT_JSON_SET_UP()\
	FPMAS_BASE_DEFAULT_JSON_SET_UP()\
	FPMAS_BASE_DEFAULT_DATAPACK_SET_UP()

/**
 * Sets up object pack based Agent serialization rules.
 *
 * This macro must be invoked exactly once from a **source** file and should
 * not be called together with FPMAS_JSON_SET_UP().
 * 
 * It is possible to call the FPMAS_DEFAULT_DATAPACK_SET_UP() macro to set up the
 * object pack serialization without specifying any Agent.
 *
 * The same set of \Agent types must be registered at runtime using the
 * FPMAS_REGISTER_AGENT_TYPES(...) macro at runtime.
 *
 * @par Example
 * ```cpp
 * #define AGENT_TYPES Agent1, Agent2, OtherAgent
 *
 * FPMAS_DATAPACK_SET_UP(AGENT_TYPES)
 *
 * int main(int argc, char** argv) {
 * 	FPMAS_REGISTER_AGENT_TYPES(AGENT_TYPES);
 *
 * 	fpmas::init(argc, argv);
 *
 * 	...
 *
 * 	fpmas::finalize();
 * }
 * ```
 *
 * ObjectPack Agent serialization is set up so that Agent are serialized into
 * JSON, stored in an ObjectPack field.
 * To prevent this behavior, FPMAS_BASE_JSON_SET_UP() and
 * FPMAS_BASE_DATAPACK_SET_UP() can be used together to enable both Agent
 * serialization techniques.
 *
 * Notice that all of this only affect **AgentPtr serialization**:
 * serialization of any other object, with JSON or ObjectPack, is not altered
 * by those macros.
 *
 * See the fpmas::io::datapack::Serializer<api::utils::PtrWrapper<AgentType>>
 * documentation to learn how to enable Agent datapack serialization.
 */
#define FPMAS_DATAPACK_SET_UP(...)\
	FPMAS_BASE_DATAPACK_SET_UP(__VA_ARGS__)\
	FPMAS_AGENT_PTR_JSON_DATAPACK_FALLBACK()

/**
 * Can be used instead of FPMAS_DATAPACK_SET_UP() to set up object pack
 * serialization without specifying any Agent type.
 */
#define FPMAS_DEFAULT_DATAPACK_SET_UP()\
	FPMAS_BASE_DEFAULT_DATAPACK_SET_UP()\
	FPMAS_BASE_DEFAULT_JSON_SET_UP()

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

namespace fpmas { 
	template<typename _T, typename... T>
		void register_types();

	/**
	 * Register_types recursion base case.
	 */
	template<> void register_types<void>();

	/**
	 * Recursive function template used to register a set of types into the
	 * adl_serializer<std::type_index> and
	 * io::datapack::base_io<std::type_index>, so that their corresponding
	 * std::type_index can be serialized.
	 *
	 * The last type specified **MUST** be void for the recursion to be valid.
	 *
	 * @par Example
	 * ```cpp
	 * fpmas::register_types<Type1, Type2, void>();
	 * ```
	 *
	 * The FPMAS_REGISTER_AGENT_TYPES(...) macro can be used instead as a more expressive
	 * and user-friendly way to register types.
	 */
	template<typename _T, typename... T>
		void register_types() {
			nlohmann::adl_serializer<std::type_index>::register_type(typeid(_T));
			fpmas::io::datapack::Serializer<std::type_index>::register_type(typeid(_T));

			register_types<T...>();
		}
}

#endif
