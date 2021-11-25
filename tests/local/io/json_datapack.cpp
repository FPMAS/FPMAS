#include "fpmas/io/json_datapack.h"

#include "gmock/gmock.h"

struct BaseData {
	int data;
	int very_important_data;
};

/*
 * Arbitrary data serializable using only json
 */
struct JsonData : public BaseData {
};

namespace nlohmann {
	template<>
		struct adl_serializer<JsonData> {
			static void to_json(json& j, const JsonData& data) {
				j = {data.data, data.very_important_data};
			}

			static void from_json(const json& j, JsonData& data) {
				data.data = j[0].get<int>();
				data.very_important_data = j[1].get<int>();
			}
		};
}

namespace fpmas { namespace io { namespace json {
	template<>
		struct light_serializer<JsonData> {
			static void to_json(light_json& j, const JsonData& data) {
				j = data.very_important_data;
			}
			static void from_json(const light_json& j, JsonData& data) {
				data.very_important_data = j.get<int>();
			}
		};
}}}

TEST(JsonDatapackConversion, json_to_datapack) {
	JsonData data;
	data.data = 67;
	data.very_important_data = 8;
	fpmas::io::datapack::JsonPack pack = data;

	data = pack.get<JsonData>();
	ASSERT_EQ(data.data, 67);
	ASSERT_EQ(data.very_important_data, 8);
}

TEST(JsonDatapackConversion, light_json_to_datapack) {
	JsonData data;
	data.data = 67;
	data.very_important_data = 8;
	fpmas::io::datapack::JsonPack pack = data;
	fpmas::io::datapack::LightJsonPack light_pack = data;
	ASSERT_LT(light_pack.data().size, pack.data().size);

	data = light_pack.get<JsonData>();
	ASSERT_EQ(data.very_important_data, 8);
}

/*
 * Arbitrary data serializable using only object pack
 */
class DatapackData : public BaseData {
};

namespace fpmas { namespace io { namespace datapack {
	template<>
		struct Serializer<DatapackData> {
			static void to_datapack(ObjectPack& p, const DatapackData& data) {
				p.allocate(2*pack_size<int>());
				p.write(data.data);
				p.write(data.very_important_data);
			}

			static DatapackData from_datapack(const ObjectPack& p) {
				DatapackData data;
				data.data = p.read<int>();
				data.very_important_data = p.read<int>();
				return data;
			}
		};

	template<>
		struct LightSerializer<DatapackData> {
			static void to_datapack(LightObjectPack& p, const DatapackData& data) {
				p = data.very_important_data;
			}

			static DatapackData from_datapack(const LightObjectPack& p) {
				DatapackData data;
				data.very_important_data = p.get<int>();
				return data;
			}
		};
}}}

TEST(JsonDatapackConversion, datapack_to_json) {
	DatapackData data;
	data.data = 67;
	data.very_important_data = 8;
	fpmas::io::json::datapack_json json = data;

	//data = json.get<DatapackData>();
	//ASSERT_EQ(data.data, 67);
	//ASSERT_EQ(data.very_important_data, 8);
}

TEST(JsonDatapackConversion, light_datapack_to_json) {
	DatapackData data;
	data.data = 67;
	data.very_important_data = 8;
	fpmas::io::json::datapack_json json = data;
	fpmas::io::json::light_datapack_json light_json = data;
	ASSERT_LT(light_json.dump().size(), json.dump().size());

	data = light_json.get<DatapackData>();
	ASSERT_EQ(data.very_important_data, 8);
}
