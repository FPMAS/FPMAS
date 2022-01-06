#include "fpmas/io/json.h"
#include "fpmas/io/datapack.h"

struct DefaultConstructibleData {
	int i = 0;

	DefaultConstructibleData() = default;
	DefaultConstructibleData(int i) : i(i) {
	}
};

struct NonDefaultConstructibleData {
	int i;
	NonDefaultConstructibleData(int i) : i(i) {}
};

struct DefaultConstructibleSerializerOnly : public DefaultConstructibleData{};

struct NonDefaultConstructibleSerializerOnly : public NonDefaultConstructibleData {
	using NonDefaultConstructibleData::NonDefaultConstructibleData;
};

namespace nlohmann {
	template<>
		struct adl_serializer<DefaultConstructibleData> {
			static void to_json(json& j, const DefaultConstructibleData& data);
			static void from_json(const json& j, DefaultConstructibleData& data);
		};

	template<>
		struct adl_serializer<DefaultConstructibleSerializerOnly> {
			template<typename JsonType>
			static void to_json(JsonType& j, const DefaultConstructibleSerializerOnly& data) {
				j = data.i;
			}

			template<typename JsonType>
			static void from_json(const JsonType& j, DefaultConstructibleSerializerOnly& data) {
				data.i = j.template get<int>();
			}
		};

	template<>
		struct adl_serializer<NonDefaultConstructibleSerializerOnly> {
			template<typename JsonType>
			static void to_json(JsonType& j, const NonDefaultConstructibleSerializerOnly& data) {
				j = data.i;
			}

			template<typename JsonType>
			static NonDefaultConstructibleSerializerOnly from_json(const JsonType& j) {
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

namespace fpmas { namespace io { namespace datapack {
	template<>
		struct Serializer<NonDefaultConstructibleSerializerOnly> {
			template<typename PackType>
				static std::size_t size(const PackType& p, const NonDefaultConstructibleSerializerOnly&) {
					return p.template size<int>();
				}

			template<typename PackType>
			static void to_datapack(PackType& p, const NonDefaultConstructibleSerializerOnly& data) {
				p = data.i;
			}

			template<typename PackType>
			static NonDefaultConstructibleSerializerOnly from_datapack(const PackType& p) {
				return {p.template get<int>()};
			}
		};

	template<>
		struct Serializer<DefaultConstructibleData> {
			static std::size_t size(const ObjectPack&, const DefaultConstructibleData& data);
			static void to_datapack(ObjectPack& pack, const DefaultConstructibleData& data);
			static DefaultConstructibleData from_datapack(const ObjectPack& pack);
		};

	template<>
		struct LightSerializer<DefaultConstructibleData> {
			static std::size_t size(const LightObjectPack&, const DefaultConstructibleData& data);
			static void to_datapack(LightObjectPack& pack, const DefaultConstructibleData& data);
			static DefaultConstructibleData from_datapack(const LightObjectPack& pack);
		};

	template<>
		struct Serializer<NonDefaultConstructibleData> {
			static std::size_t size(const ObjectPack&, const NonDefaultConstructibleData& data);
			static void to_datapack(ObjectPack& pack, const NonDefaultConstructibleData& data);
			static NonDefaultConstructibleData from_datapack(const ObjectPack& pack);
		};

	template<>
		struct LightSerializer<NonDefaultConstructibleData> {
			static std::size_t size(const LightObjectPack& pack, const NonDefaultConstructibleData& data);
			static void to_datapack(LightObjectPack& pack, const NonDefaultConstructibleData& data);
			static NonDefaultConstructibleData from_datapack(const LightObjectPack& pack);
		};
}}}

