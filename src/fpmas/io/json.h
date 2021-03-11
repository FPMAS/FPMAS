#ifndef FPMAS_JSON_H
#define FPMAS_JSON_H

#include "nlohmann/json.hpp"

namespace fpmas { namespace io { namespace json {
	template<typename T>
		struct light_serializer {
			static void to_json(nlohmann::json& j, const T& data) {
				j = data;
			}

			static T from_json(const nlohmann::json& j) {
				return j.get<T>();
			}
		};
}}}
#endif
