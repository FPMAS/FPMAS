#ifndef FPMAS_JSON_H
#define FPMAS_JSON_H

#include "nlohmann/json.hpp"

namespace fpmas { namespace io { namespace json {
	template<typename T, typename> struct light_serializer;

	/**
	 * Light json type definition, that might be used to produce "lighter" json
	 * than the regular nlohmann::json version.
	 *
	 * As an example, the light_json serialization of a \DistributedNode might
	 * be used to only serialize minimalist \DistributedNode information,
	 * such as its ID, without considering its weight or data, that are
	 * optional to insert a \DistributedNode into a \DistributedGraph.
	 	 *
	 * @see light_serializer
	 * @see \ref light_serializer_DistributedNode "light_serializer< NodePtrWrapper< T > >"
	 */
	typedef nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, double, std::allocator, light_serializer> light_json;

	/**
	 * Default `light_serializer` implementation, that automatically falls back
	 * to the classic `nlohmann::adl_serializer`. In consequence, the
	 * `light_serializer` is effective only when explicit specialization are
	 * provided (see for example \ref light_serializer_DistributedNode
	 * "light_serializer<NodePtrWrapper<T>>").
	 *
	 * The definition of a custom `json` type, `light_json`, and the associated
	 * serializer, `json_serializer`, allows to easily reuse classic
	 * `adl_serializer` implementations when required.
	 *
	 * For example, the serialization of a \DistributedId is the same for a
	 * classic
	 * `nlohmann::json` or for a `light_json`, so we shall not need to provide
	 * two implementations with the same serialization rules. This is indeed
	 * not required, since nlohmann::adl_serializer<DistributedId> serialization rules
	 * are provided using templated `to_json` and `from_json` methods:
	 * ```cpp
	 * namespace nlohmann {
	 * 	template<>
	 * 		struct adl_serializer<DistributedId> {
	 * 			template<typename JsonType>
	 * 				static void to_json(JsonType& j, const DistributedId& id) {
	 * 					...
	 * 				}
	 * 			template<typename JsonType>
	 * 				static void from_json(const JsonType& j, DistributedId& id) {
	 * 					...
	 * 				}
	 * 		};
	 * }
	 * ```
	 *
	 * So, when it is required to serialize a \DistributedId in the
	 * `light_json` serialization process, the method above are automatically
	 * instantiated so that the same serialization rules are used for any
	 * `JsonType`.
	 *
	 * An other interesting use case occurs with \DistributedEdges, where we
	 * typically want to use a `light_json`, so that source and target nodes
	 * are transmitted using a `light_json`: there is no need to serialize and
	 * transmit node data to instantiate a \DistributedEdge.
	 *
	 * However, the \DistributedEdge serialization rules defined in
	 * \ref adl_serializer_DistributedEdge "adl_serializer<EdgePtrWrapper<T>>"
	 * are the same for **any JsonType**, but that's not the case for the
	 * \DistributedNode, where two serializer implementations are provided:
	 * - \ref adl_serializer_DistributedNode "adl_serializer<NodePtrWrapper<T>>" : complete
	 *   json serialization, used to transmit nodes with their data.
	 * - \ref light_serializer_DistributedNode
	 *   "light_serializer<NodePtrWrapper<T>>" : partial json serialization,
	 *   with minimalist node information.
	 *
	 * So even if the implementation of the \DistributedEdge is unique, the
	 * proper \DistributedNode serialization rules are automatically selected
	 * at compile time according to the provided `JsonType`.
	 *
	 * Concretely, \DistributedEdges can be serialized more efficiently using a
	 * `light_json`: see \ref TypedMpiDistributedEdge "TypedMpi<DistributedEdge<T>>".
	 *
	 * @tparam T type to serialize
	 * @tparam Enable SFINAE condition
	 */
	template<typename T, typename Enable = void>
		struct light_serializer {
			/**
			 * Serializes `data` into `j` using the regular
			 * `nlohmann::adl_serializer` implementation for `T`.
			 *
			 * Equivalent to:
			 * ```cpp
			 * nlohmann::adl_serializer<T>::to_json(j, data);
			 * ```
			 *
			 * @param j json output
			 * @param data data to serialize
			 */
			static void to_json(light_json& j, const T& data) {
				nlohmann::adl_serializer<T>::to_json(j, data);
			}

			/**
			 * Unserializes `data` from `j` using the regular
			 * `nlohmann::adl_serializer` implementation for `T`.
			 *
			 * Equivalent to:
			 * ```cpp
			 * nlohmann::adl_serializer<T>::from_json(j, data);
			 * ```
			 *
			 * @param j json input
			 * @param data data output
			 */
			static void from_json(const light_json& j, T& data) {
				nlohmann::adl_serializer<T>::from_json(j, data);
			}
		};

	/**
	 * light_serializer specialization for non default constructible types,
	 * still based on `nlohmann::adl_serializer`, as explained in the
	 * light_serializer documentation.
	 *
	 * @tparam T a non default constructible type to serialize
	 */
	template<typename T>
		struct light_serializer<T, typename std::enable_if<!std::is_default_constructible<T>::value>::type> {
			/**
			 * Serializes `data` into `j` using the regular
			 * `nlohmann::adl_serializer` implementation for `T`.
			 *
			 * Equivalent to:
			 * ```cpp
			 * nlohmann::adl_serializer<T>::to_json(j, data);
			 * ```
			 *
			 * @param j json output
			 * @param data data to serialize
			 */
			static void to_json(light_json& j, const T& data) {
				nlohmann::adl_serializer<T>::to_json(j, data);
			}

			/**
			 * Unserializes non default constructible `data` from `j` using the
			 * regular `nlohmann::adl_serializer` implementation for `T`.
			 *
			 * Equivalent to:
			 * ```cpp
			 * return nlohmann::adl_serializer<T>::from_json(j);
			 * ```
			 *
			 * @param j json input
			 * @return unserialized data
			 */
			static T from_json(const light_json& j) {
				return nlohmann::adl_serializer<T>::from_json(j);
			}
		};
	}}}
#endif
