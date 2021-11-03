#include "fpmas/io/datapack_base_io.h"
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

TEST(datapack_base_io, set) {
	TEST_DATAPACK_BASE_IO(
			std::set<double>,
			{3.4, 192000.8, 7},
			{8, 17, 4.4, 12.1}
			);
}

TEST(datapack_base_io, list) {
	TEST_DATAPACK_BASE_IO(
			std::list<std::int64_t>,
			{1, 2, 3},
			{-64, 15, -30000, 2}
			);
};

TEST(datapack_base_io, pair) {
	typedef std::pair<float, std::string> TestPair;
	TEST_DATAPACK_BASE_IO(
			TestPair,
			{8.6f, "hello"},
			{9.5f, "world"}
			);
};

TEST(datapack_base_io, unordered_map) {
	typedef std::unordered_map<std::uint32_t, std::string> TestMap;
	TEST_DATAPACK_BASE_IO(
			TestMap,
			{{1, "hello"}, {74, "world"}},
			{{0, "foo"}, {1, "bar"}, {122, "baz"}}
			);
}

TEST(datapack_base_io, array) {
	typedef std::array<std::string, 3> TestArray;
	TEST_DATAPACK_BASE_IO(
			TestArray,
			{{"a", "b", "cdef"}},
			{{"foo", "bar", "baz"}}
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

