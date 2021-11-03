#ifndef FPMAS_MODEL_SERIALIZER_H
#define FPMAS_MODEL_SERIALIZER_H

#include "json_serializer.h"
#include "datapack_serializer.h"

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
}

#endif
