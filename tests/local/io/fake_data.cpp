#include "fake_data.h"

namespace nlohmann {
	void adl_serializer<DefaultConstructibleData>::to_json(
			json& j, const DefaultConstructibleData& data) {
		j = data.i;
	}

	void adl_serializer<DefaultConstructibleData>::from_json(
			const json& j, DefaultConstructibleData& data) {
		data.i = j.get<int>();
	}
}

namespace fpmas { namespace io { namespace json {
	void light_serializer<DefaultConstructibleData>::to_json(
			light_json&, const DefaultConstructibleData&) {}

	void light_serializer<DefaultConstructibleData>::from_json(
			const light_json&, DefaultConstructibleData&) {}

	void light_serializer<NonDefaultConstructibleData>::to_json(
			light_json&, const NonDefaultConstructibleData&) {}
	NonDefaultConstructibleData light_serializer<NonDefaultConstructibleData>::from_json(
			const light_json&) {
		return NonDefaultConstructibleData(0);
	}
}}}

namespace fpmas { namespace io { namespace datapack {
	void Serializer<DefaultConstructibleData>::to_datapack(ObjectPack &pack, const DefaultConstructibleData &data) {
		pack.allocate(pack_size<int>());
		pack.write(data.i);
	}

	DefaultConstructibleData Serializer<DefaultConstructibleData>::from_datapack(const ObjectPack &pack) {
		int i;
		pack.read(i);
		DefaultConstructibleData data;
		data.i = i;
		return data;
	}

	void LightSerializer<DefaultConstructibleData>::to_datapack(LightObjectPack &, const DefaultConstructibleData &) {
	}

	DefaultConstructibleData LightSerializer<DefaultConstructibleData>::from_datapack(const LightObjectPack &) {
		return {};
	}

	void Serializer<NonDefaultConstructibleData>::to_datapack(ObjectPack &pack, const NonDefaultConstructibleData &data) {
		pack.allocate(pack_size<int>());
		pack.write(data.i);
	}

	NonDefaultConstructibleData Serializer<NonDefaultConstructibleData>::from_datapack(const ObjectPack &pack) {
		int i;
		pack.read(i);
		return {i};
	}

	void LightSerializer<NonDefaultConstructibleData>::to_datapack(LightObjectPack &, const NonDefaultConstructibleData &) {
	}

	NonDefaultConstructibleData LightSerializer<NonDefaultConstructibleData>::from_datapack(const LightObjectPack &) {
		return {0};
	}

}}}

