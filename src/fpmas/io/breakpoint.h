#ifndef FPMAS_BREAKPOINT_H
#define FPMAS_BREAKPOINT_H

/**
 * \file src/fpmas/io/breakpoint.h
 *
 * fpmas::api::io::Breakpoint implementations.
 */

#include <iostream>

#include "fpmas/api/io/breakpoint.h"
#include "nlohmann/json.hpp"

namespace fpmas { namespace io {

	/**
	 * JSON based api::io::Breakpoint implementation.
	 *
	 * Data is serialized / unserialized using [nlohmann
	 * json](https://github.com/nlohmann/json) serialization rules.
	 *
	 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
	 */
	template<typename T>
		class Breakpoint : public api::io::Breakpoint<T> {
			public:
				/**
				 * Serializes the specified `object` with the [nlohmann
				 * json](https://github.com/nlohmann/json) library, and sends it to the
				 * output `stream`.
				 *
				 * The [MessagePack](https://msgpack.org/) format is used for
				 * more compact dumps.
				 *
				 * @param stream output stream
				 * @param object object to save
				 */
				void dump(std::ostream& stream, const T& object) override {
					nlohmann::json j;
					nlohmann::json::json_serializer<T, void>::to_json(j, object);
					std::vector<uint8_t> data = nlohmann::json::to_msgpack(j);
					for(auto i : data)
						// Conversion from uint8_t to char
						stream.put(i);
				}

				/**
				 * Loads data from the input `stream` into the specified
				 * `object`.
				 *
				 * The stream must provide Json data in the
				 * [MessagePack](https://msgpack.org/) format, as performed by
				 * the dump() method.
				 *
				 * Read data is then loaded into `object` using the following
				 * statement, where `j` is the json object parsed from the
				 * input stream:
				 * ```cpp
				 * nlohmann::json::json_serializer<T, void>::from_json(j, object);
				 * ```
				 *
				 * @param stream input stream
				 * @param object T instance in which read data will be loaded
				 */
				void load(std::istream& stream, T& object) override {
					std::vector<uint8_t> data;
					// Conversion from char to uint8_t
					for(char i; stream.get(i);)
						data.push_back(i);

					nlohmann::json j = nlohmann::json::from_msgpack(data);
					nlohmann::json::json_serializer<T, void>::from_json(j, object);
				}
		};
}}
#endif
