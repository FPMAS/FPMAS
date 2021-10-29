#include "fpmas/io/datapack.h"
#include "fpmas/random/random.h"

#include "gmock/gmock.h"

using namespace ::testing;

namespace fpmas { namespace io { namespace datapack {
	template<template<typename> class S>
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

TEST(datapack_base_io, int64) {
	TEST_DATAPACK_BASE_IO(std::int64_t, -45999, 45721);
}
TEST(datapack_base_io, double) {
	TEST_DATAPACK_BASE_IO(double, -45e201, 78);
}
TEST(datapack_base_io, string) {
	TEST_DATAPACK_BASE_IO(std::string, "abcd", "hello");
}
TEST(datapack_base_io, DistributedId) {
	TEST_DATAPACK_BASE_IO(DistributedId, {8, 421}, {2, 14});
}
TEST(datapack_base_io, size_vector) {
	TEST_DATAPACK_BASE_IO(
			std::vector<std::size_t>,
			{34, 25000, 560},
			{1, 2, 72, 42}
			);
}

TEST(datapack_base_io, datapack) {
	std::array<DataPack, 2> data = {{
		DataPack(12, 1),
		DataPack(8, 1)
	}};

	fpmas::random::UniformIntDistribution<char> rd_char(0, 255);
	fpmas::random::mt19937_64 rd;
	for(auto item : data)
		for(std::size_t i = 0; i < item.size; i++)
			item.buffer[i] = rd_char(rd);

	TEST_DATAPACK_BASE_IO(DataPack, data[0], data[1]);
}

TEST(datapack_base_io, datapack_vector) {
	std::array<std::vector<DataPack>, 2> data = {{
		{DataPack(12, 1), DataPack(10, 1), DataPack(7, 1)},
		{DataPack(8, 1)}
	}};

	fpmas::random::UniformIntDistribution<char> rd_char(0, 255);
	fpmas::random::mt19937_64 rd;
	for(auto vec : data)
		for(auto item : vec)
			for(std::size_t i = 0; i < item.size; i++)
				item.buffer[i] = rd_char(rd);

	TEST_DATAPACK_BASE_IO(std::vector<DataPack>, data[0], data[1]);
}

TEST(datapack_base_io, empty_vec) {
	std::vector<int> vec;
	DataPack pack(pack_size(vec), 1);
	std::size_t offset = 0;
	write(pack, vec, offset);

	std::vector<int> read_vec;
	offset = 0;
	read(pack, read_vec, offset);

	ASSERT_THAT(read_vec, IsEmpty());
}

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
