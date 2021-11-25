#ifndef FPMAS_IO_JSON_DATAPACK_H
#define FPMAS_IO_JSON_DATAPACK_H

/**
 * @file src/fpmas/io/json_datapack.h
 *
 * Defines structures to serialize Json into ObjectPacks, and ObjectPacks into
 * Json.
 */

#include "datapack.h"
#include "json.h"

namespace fpmas { namespace io { namespace datapack {
		/**
		 * A Serializer implementation based on json serialization.
		 *
		 * Objects of type T are serialized using the nlohmann::json
		 * library. Json strings are then written and read directly to
		 * DataPack buffers.
		 *
		 * More particularly, the type T must be serializable into the provided
		 * `JsonType`, that is itself based on `nlohmann::basic_json`. For
		 * example, if `JsonType` is `nlohmann::json`, the classical
		 * nlohmann::json custom serialization rules must be provided as
		 * explained at
		 * https://github.com/nlohmann/json#arbitrary-types-conversions.
		 *
		 * An fpmas::io::json::light_serializer must be provided for T when
		 * `JsonType` is fpmas::io::json::light_json. See fpmas::io::json for
		 * more information.
		 *
		 *
		 * @tparam T type to serialize
		 * @tparam JsonType json type used for serialization
		 * (nlohmann::json or fpmas::io::json::light_json).
		 */
		template<typename T, typename JsonType>
			struct BasicJsonSerializer {
				/**
				 * Serializes `data` to `pack`.
				 *
				 * An std::string instance is produced using
				 * `JsonType(data).dump()` and written to `pack`.
				 *
				 * @param pack destination pack
				 * @param data data to serialize
				 */
				template<typename PackType>
					static void to_datapack(PackType& pack, const T& data) {
						std::string json_str = JsonType(data).dump();
						pack.allocate(pack_size(json_str));
						pack.write(json_str);
					}

				/**
				 * Unserializes data from `pack`.
				 *
				 * A json std::string is retrieved from the pack and parsed
				 * using `JsonType::parse()`, and an instance of T is
				 * unserialized using the `JsonType::get()` method.
				 *
				 * @param pack source pack
				 * @return unserialized data
				 */
				template<typename PackType>
					static T from_datapack(const PackType& pack) {
						return JsonType::parse(
								pack.template read<std::string>()
								).template get<T>();
					}
			};

		/**
		 * An nlohmann::json based Serializer.
		 *
		 * See https://github.com/nlohmann/json#arbitrary-types-conversions to
		 * learn how to define rules to serialize **any custom type** with the
		 * nlohmann::json library.
		 *
		 * @tparam T type to serialize as an nlohmann::json instance
		 */
		template<typename T, typename Enable = void>
			using JsonSerializer = BasicJsonSerializer<T, nlohmann::json>;
		/**
		 * An nlohmann::json based BasicObjectPack.
		 */
		typedef BasicObjectPack<JsonSerializer> JsonPack;

		/**
		 * An fpmas::io::json::light_json based serializer. Can be
		 * considered as the _light_ version of JsonSerializer.
		 */
		template<typename T, typename Enable = void>
			using LightJsonSerializer = BasicJsonSerializer<T, fpmas::io::json::light_json>;
		/**
		 * An fpmas::io::json::light_json based BasicObjectPack. Can be
		 * considered as the _light_ version of JsonPack.
		 */
		typedef BasicObjectPack<LightJsonSerializer> LightJsonPack;
}}}

namespace fpmas { namespace io { namespace json {
	/**
	 * An nlohmann serializer implementation based on object pack
	 * serialization.
	 *
	 * Objects of type T are serialized using the fpmas::io::datapack
	 * library. ObjectPacks are then written and read directly to
	 * json as strings.
	 *
	 * More particularly, the type T must be serializable into the provided
	 * `PackType`, that is itself based on
	 * fpmas::io::datapack::BasicObjectPack. For example, if `PackType` is
	 * fpmas::io::datapack::ObjectPack, the classical
	 * fpmas::io::datapack::Serializer custom serialization rules must be
	 * provided.
	 *
	 * An fpmas::io::datapack::LightSerializer specialization must be provided
	 * for T when `PackType` is fpmas::io::datapack::LightObjectPack. See
	 * fpmas::io::datapack for more information.
	 *
	 *
	 * @tparam T type to serialize
	 * @tparam PackType object pack type used for serialization
	 * (fpmas::io::datapack::ObjectPack,
	 * fpmas::io::datapack::LightObjectPack,...)
	 */
	template<typename T, typename PackType>
		struct basic_datapack_serializer {
			/**
			 * Serializes `data` to `json`.
			 *
			 * An std::string instance is produced from the DataPack generated
			 * by `PackType(data)` and written to `json`.
			 *
			 * @param json destination json
			 * @param data data to serialize
			 */
			template<typename JsonType>
				static void to_json(JsonType& json, const T& data) {
					PackType p = data;
					std::vector<std::uint8_t> binary_buffer(p.data().size);
					for(std::size_t i = 0; i < p.data().size; i++)
						binary_buffer[i] = p.data().buffer[i];

					json = JsonType::binary(std::move(binary_buffer));
				}

			/**
			 * Unserializes data from `json`.
			 *
			 * A raw DataPack is retrieved from the pack and parsed
			 * using `PackType::parse()`, and an instance of T is
			 * unserialized using the `PackType::get()` method.
			 *
			 * @param json source json
			 * @return unserialized data
			 */
			template<typename JsonType>
				static T from_json(const JsonType& json) {
					const std::vector<std::uint8_t>& binary_buffer = json.get_binary();

					datapack::DataPack p(binary_buffer.size(), 1);
					for(std::size_t i = 0; i < binary_buffer.size(); i++)
						p.buffer[i] = binary_buffer[i];
					return PackType::parse(p).template get<T>();
				}
		};

	/**
	 * An fpmas::io::datapack::ObjectPack based nlohmann::json serializer.
	 *
	 * @tparam T type to serialize as an ObjectPack instance
	 */
	template<typename T, typename Enable=void>
		using datapack_serializer
		= basic_datapack_serializer<T, fpmas::io::datapack::ObjectPack>;

	/**
	 * An ObjectPack based nlohmann::basic_json.
	 */
	typedef nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, double, std::allocator, datapack_serializer> datapack_json;

	/**
	 * An fpmas::io::datapack::LightObjectPack based nlohmann::json serializer.
	 * Can be considered as the _light_ version of datapack_serializer.
	 */
	template<typename T, typename Enable=void>
		using light_datapack_serializer
		= basic_datapack_serializer<T, fpmas::io::datapack::LightObjectPack>;

	/**
	 * An fpmas::io::datapack::LightObjectPack based nlohmann::basic_json. Can be
	 * considered as the _light_ version of datapack_json.
	 */
	typedef nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, double, std::allocator, light_datapack_serializer> light_datapack_json;

}}}

#endif
