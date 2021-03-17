#include "fpmas/io/json.h"

struct DefaultConstructibleData {
	int i = 0;
};

struct NonDefaultConstructibleData {
	int i;
	NonDefaultConstructibleData(int i) : i(i) {}
};

struct DefaultConstructibleAdlOnly : public DefaultConstructibleData{};

struct NonDefaultConstructibleAdlOnly : public NonDefaultConstructibleData {
	using NonDefaultConstructibleData::NonDefaultConstructibleData;
};

namespace nlohmann {
	template<>
		struct adl_serializer<DefaultConstructibleData> {
			static void to_json(json& j, const DefaultConstructibleData& data);
			static void from_json(const json& j, DefaultConstructibleData& data);
		};

	template<>
		struct adl_serializer<DefaultConstructibleAdlOnly> {
			template<typename JsonType>
			static void to_json(JsonType& j, const DefaultConstructibleAdlOnly& data) {
				j = data.i;
			}

			template<typename JsonType>
			static void from_json(const JsonType& j, DefaultConstructibleAdlOnly& data) {
				data.i = j.template get<int>();
			}
		};

	template<>
		struct adl_serializer<NonDefaultConstructibleAdlOnly> {
			template<typename JsonType>
			static void to_json(JsonType& j, const NonDefaultConstructibleAdlOnly& data) {
				j = data.i;
			}

			template<typename JsonType>
			static NonDefaultConstructibleAdlOnly from_json(const JsonType& j) {
				return {j.template get<int>()};
			}
		};
}

namespace fpmas { namespace io { namespace json {
	template<>
		struct light_serializer<DefaultConstructibleData> {
			static void to_json(light_json&, const DefaultConstructibleData&);
			static void from_json(const light_json&, DefaultConstructibleData&);
		};
	template<>
		struct light_serializer<NonDefaultConstructibleData> {
			static void to_json(light_json&, const NonDefaultConstructibleData&);
			static NonDefaultConstructibleData from_json(const light_json&);
		};
}}}

