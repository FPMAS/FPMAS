#include "fpmas/io/datapack.h"
#include "fpmas/random/random.h"

#include "gmock/gmock.h"

using namespace ::testing;

namespace fpmas { namespace io { namespace datapack {
	template<template<typename, typename> class S>
	bool operator==(const BasicObjectPack<S>& o1, const BasicObjectPack<S>& o2) {
		return o1.data() == o2.data();
	}
}}}

using namespace fpmas::io::datapack;

#define TEST_DATAPACK_BASE_IO(TYPE, ...)\
	std::array<TYPE, 2> _data = {{ __VA_ARGS__ }};\
	DataPack pack(pack_size(_data[0]) + pack_size(_data[1]), 1);\
	std::size_t offset = 0;\
	write(pack, _data[0], offset);\
	write(pack, _data[1], offset);\
	offset = 0;\
	std::array<TYPE, 2> read_data;\
	read(pack, read_data[0], offset);\
	read(pack, read_data[1], offset);\
	ASSERT_THAT(read_data, ElementsAreArray(_data));


TEST(datapack_base_io, objectpack) {
	ObjectPack o1;
	o1.allocate(pack_size<double>());
	o1.write(18.7);
	DataPack o1_pack = o1.dump();

	ObjectPack o2;
	o2.allocate(pack_size<std::int32_t>());
	o2.write(-30434);
	DataPack o2_pack = o2.dump();

	ObjectPack o3;
	o3.allocate(pack_size<float>());
	o3.write(-2.4f);

	ObjectPack objectpack;
	std::string str = "hello world";
	objectpack.allocate(pack_size(str) + pack_size(o1_pack) + pack_size(o2_pack));
	objectpack.write(str);
	objectpack.write(o1_pack);
	objectpack.write(o2_pack);

	TEST_DATAPACK_BASE_IO(ObjectPack, objectpack, o3);
}

TEST(Serializer, str) {
	std::string data = "hello world";
	ObjectPack serial_data(data);
	std::string deserial_data = serial_data.get<std::string>();

	ASSERT_EQ(data, deserial_data);
}

TEST(Serializer, str_vector) {
	std::vector<std::string> data = {"ab", "zzzzz", "678908DSDJSK"};
	ObjectPack serial_data(data);
	auto deserial_data = serial_data.get<std::vector<std::string>>();

	ASSERT_THAT(deserial_data, ElementsAreArray(data));
}
