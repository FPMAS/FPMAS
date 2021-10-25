#include "fpmas/io/datapack.h"
#include "fpmas/random/random.h"

#include "gmock/gmock.h"

using namespace ::testing;
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


TEST(Serializer, str_vector) {
	std::vector<std::string> data = {"ab", "zzzzz", "678908DSDJSK"};
	auto serial_data = Serializer<std::vector<std::string>>::to_datapack(data);
	auto deserial_data = Serializer<std::vector<std::string>>::from_datapack(serial_data);

	ASSERT_THAT(deserial_data, ElementsAreArray(data));
}
