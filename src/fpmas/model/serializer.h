#ifndef FPMAS_MODEL_SERIALIZER_H
#define FPMAS_MODEL_SERIALIZER_H

#include "json_serializer.h"
#include "datapack_serializer.h"

/**
 * Defines a json based implementation of AgentPtr Serializer and
 * LightSerializer implementation, that can be used instead of definitions
 * provided by FPMAS_DATAPACK_SET_UP().
 *
 * This allows to use the Serializer to serialize DistributedNodes for example,
 * using the json serialization only for AgentPtr.
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
 * Sets up json based Agent serialization rules.
 *
 * This macro must be invoked exactly once from a **source** file and should
 * not be called with FPMAS_DATAPACK_SET_UP().
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
	FPMAS_AGENT_PTR_DATAPACK_JSON_FALLBACK()

#define FPMAS_DATAPACK_SET_UP(...)\
	FPMAS_BASE_DATAPACK_SET_UP(__VA_ARGS__)\
	FPMAS_BASE_JSON_SET_UP(__VA_ARGS__)

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
