#include "fpmas/io/datapack.h"
#include "fpmas/random/random.h"
#include "fpmas/graph/distributed_edge.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "fake_data.h"

#include "gmock/gmock.h"

using namespace ::testing;

namespace fpmas { namespace io { namespace datapack {
	template<template<typename, typename> class S>
	bool operator==(const BasicObjectPack<S>& o1, const BasicObjectPack<S>& o2) {
		return o1.data() == o2.data();
	}
}}}

using namespace fpmas::io::datapack;

template<typename T, typename Enable = void>
struct MockSerializer {
	struct Mock {
		MOCK_METHOD(std::size_t, size, (const BasicObjectPack<MockSerializer>&, const T&));
		MOCK_METHOD(void, to_datapack, (BasicObjectPack<MockSerializer>&, const T&));
		MOCK_METHOD(T, from_datapack, (const BasicObjectPack<MockSerializer>&));
	};

	static Mock* mock;

	static std::size_t size(const BasicObjectPack<MockSerializer>& p, const T& item) {
		return mock->size(p, item);
	}

	static void to_datapack(BasicObjectPack<MockSerializer>& p, const T& item) {
		mock->to_datapack(p, item);
	}

	static T from_datapack(const BasicObjectPack<MockSerializer>& p) {
		return mock->from_datapack(p);
	}
};
template<typename T, typename Enable>
typename MockSerializer<T, Enable>::Mock* MockSerializer<T, Enable>::mock;

typedef BasicObjectPack<MockSerializer> MockObjectPack;

class BasicObjectPackTest : public Test {
	void SetUp() override {
		MockSerializer<int>::mock = new MockSerializer<int>::Mock;
	}

	void TearDown() override {
		delete MockSerializer<int>::mock;
	}
};

TEST_F(BasicObjectPackTest, allocate) {
	MockObjectPack pack;
	pack.allocate(24);
	ASSERT_EQ(pack.data().size, 24);
}

TEST_F(BasicObjectPackTest, expand) {
	MockObjectPack pack;
	pack.allocate(8);
	std::uint8_t i = 0x12;
	std::memcpy(pack.data().buffer, &i, sizeof(std::uint8_t));

	pack.expand(12);
	ASSERT_EQ(pack.data().size, 8+12);

	// Ensures already written data is unchanged
	std::memcpy(&i, pack.data().buffer, sizeof(std::uint8_t));
}

TEST_F(BasicObjectPackTest, read_write_scalar) {
	MockObjectPack pack;
	pack.allocate(sizeof(std::uint32_t));
	pack.write(0x12345678);

	std::uint32_t read;
	pack.read(read);

	ASSERT_EQ(read, 0x12345678);
}

TEST_F(BasicObjectPackTest, read_write_array) {
	std::uint16_t array[4] = {0x1111, 0x2222, 0x3333, 0x4444};
	MockObjectPack pack;
	pack.allocate(sizeof(array));
	pack.write(array, sizeof(array));

	std::uint16_t r_array[4];
	pack.read(r_array, sizeof(r_array));
	ASSERT_THAT(r_array, ElementsAreArray(array));
}

TEST_F(BasicObjectPackTest, read_offset) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);

	char c;
	pack.read(c);

	std::size_t read_offset = pack.readOffset();
	{
		// Perform a read and discard result
		std::int16_t i;
		pack.read(i);
	}
	// Set back the read cursor
	pack.seekRead(read_offset);

	// Ensures read is consistent
	std::int16_t i;
	pack.read(i);
	ASSERT_EQ(i, 0x1234);
}

TEST_F(BasicObjectPackTest, write_offset) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');

	// First write
	std::size_t offset = pack.writeOffset();
	pack.write((std::int16_t) 0x1234);

	// Overrides write
	pack.seekWrite(offset);
	pack.write((std::int16_t) 0x5678);

	// Dummy read
	char c;
	pack.read(c);

	// Reads overriden value
	std::int16_t r_i;
	pack.read(r_i);
	ASSERT_EQ(r_i, 0x5678);
}

TEST_F(BasicObjectPackTest, check) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);

	// Dummy read
	char c;
	pack.read(c);

	// Performs several checks
	for(int _ = 0; _ < 4; _++) {
		std::int16_t i;
		pack.check(i);
		ASSERT_EQ(i, 0x1234);
	}
}

TEST_F(BasicObjectPackTest, extract) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t) + sizeof(char));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);
	pack.write('b');

	// Dummy read
	char a;
	pack.read(a);

	MockObjectPack extract = pack.extract(sizeof(std::int16_t));

	// The extract operation must properly move the read cursor
	char b;
	pack.read(b);
	ASSERT_EQ(b, 'b');

	// Check extract content
	std::int16_t i;
	extract.read(i);
	ASSERT_EQ(i, 0x1234);
}

TEST_F(BasicObjectPackTest, copy_parse) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);

	DataPack datapack = pack.data();
	MockObjectPack p = MockObjectPack::parse(pack.data());

	// Source data pack is left unchanged
	for(std::size_t i = 0; i < datapack.size; i++)
		ASSERT_EQ(pack.data().buffer[i], datapack.buffer[i]);

	char a;
	std::int16_t i;
	p.read(a);
	p.read(i);

	ASSERT_EQ(a, 'a');
	ASSERT_EQ(i, 0x1234);
}

TEST_F(BasicObjectPackTest, move_parse) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);

	DataPack datapack = pack.data();
	MockObjectPack p = MockObjectPack::parse(std::move(datapack));

	// datapack is left empty
	ASSERT_EQ(datapack.size, 0);

	char a;
	std::int16_t i;
	p.read(a);
	p.read(i);

	ASSERT_EQ(a, 'a');
	ASSERT_EQ(i, 0x1234);
}

TEST_F(BasicObjectPackTest, dump) {
	MockObjectPack pack;
	pack.allocate(sizeof(char) + sizeof(std::int16_t));
	pack.write('a');
	pack.write((std::int16_t) 0x1234);

	DataPack dump = pack.dump();
	MockObjectPack p = MockObjectPack::parse(dump);

	// Internal buffer is left empty
	ASSERT_EQ(pack.data().size, 0);

	// Checks dumped data content
	char a;
	std::int16_t i;
	p.read(a);
	p.read(i);

	ASSERT_EQ(a, 'a');
	ASSERT_EQ(i, 0x1234);
}

TEST_F(BasicObjectPackTest, constructor) {
	int i = 132;
	const MockObjectPack* size_arg;
	MockObjectPack* to_datapack_arg;
	EXPECT_CALL(*MockSerializer<int>::mock, size(_, i))
		.WillOnce(Invoke([&size_arg] (const MockObjectPack& p, const int&) {
					size_arg = &p;
					return 12;
					}));
	EXPECT_CALL(*MockSerializer<int>::mock, to_datapack(_, i))
		.WillOnce(Invoke([&to_datapack_arg] (MockObjectPack& p, const int&) {
					to_datapack_arg = &p;
					}));

	MockObjectPack pack(i);
	ASSERT_EQ(size_arg, &pack);
	ASSERT_EQ(to_datapack_arg, &pack);
	ASSERT_EQ(pack.data().size, 12);
}

TEST_F(BasicObjectPackTest, assignment) {
	int i = 132;
	const MockObjectPack* size_arg;
	MockObjectPack* to_datapack_arg;
	EXPECT_CALL(*MockSerializer<int>::mock, size(_, i))
		.WillOnce(Invoke([&size_arg] (const MockObjectPack& p, const int&) {
					size_arg = &p;
					return 12;
					}));
	EXPECT_CALL(*MockSerializer<int>::mock, to_datapack(_, i))
		.WillOnce(Invoke([&to_datapack_arg] (MockObjectPack& p, const int&) {
					to_datapack_arg = &p;
					}));

	MockObjectPack pack;
	pack = i;

	ASSERT_EQ(size_arg, &pack);
	ASSERT_EQ(to_datapack_arg, &pack);
	ASSERT_EQ(pack.data().size, 12);
}

TEST_F(BasicObjectPackTest, put) {
	int i = 132;
	MockObjectPack* to_datapack_arg;
	EXPECT_CALL(*MockSerializer<int>::mock, to_datapack(_, i))
		.WillOnce(Invoke([&to_datapack_arg] (MockObjectPack& p, const int&) {
					to_datapack_arg = &p;
					}));

	MockObjectPack pack;
	pack.put(i);
	ASSERT_EQ(to_datapack_arg, &pack);
}

TEST_F(BasicObjectPackTest, get) {
	MockObjectPack pack;

	const MockObjectPack* from_datapack_arg;
	EXPECT_CALL(*MockSerializer<int>::mock, from_datapack(_))
		.WillOnce(Invoke([&from_datapack_arg] (const MockObjectPack& p) {
					from_datapack_arg = &p;
					return 666;
					}));

	ASSERT_EQ(pack.get<int>(), 666);
	ASSERT_EQ(from_datapack_arg, &pack);
}

#define TEST_DATAPACK_IO(TYPE, ...)\
	std::array<TYPE, 2> _data = {{ __VA_ARGS__ }};\
	ObjectPack pack;\
	pack.allocate(pack.size(_data[0]) + pack.size(_data[1]));\
	pack.put(_data[0]);\
	pack.put(_data[1]);\
	std::array<TYPE, 2> read_data;\
	read_data[0] = pack.get<TYPE>();\
	read_data[1] = pack.get<TYPE>();\
	ASSERT_THAT(read_data, ElementsAreArray(_data));

TEST(ObjectPack, int64) {
	TEST_DATAPACK_IO(std::int64_t, -45999, 45721);
}
TEST(ObjectPack, double) {
	TEST_DATAPACK_IO(double, -45e201, 78);
}
TEST(ObjectPack, string) {
	TEST_DATAPACK_IO(std::string, "abcd", "hello");
}
TEST(ObjectPack, DistributedId) {
	TEST_DATAPACK_IO(DistributedId, {8, 421}, {2, 14});
}
TEST(ObjectPack, size_vector) {
	TEST_DATAPACK_IO(
			std::vector<std::size_t>,
			{34, 25000, 560},
			{1, 2, 72, 42}
			);
}

TEST(ObjectPack, set) {
	TEST_DATAPACK_IO(
			std::set<double>,
			{3.4, 192000.8, 7},
			{8, 17, 4.4, 12.1}
			);
}

TEST(ObjectPack, list) {
	TEST_DATAPACK_IO(
			std::list<std::int64_t>,
			{1, 2, 3},
			{-64, 15, -30000, 2}
			);
};

TEST(ObjectPack, deque) {
	TEST_DATAPACK_IO(
			std::deque<std::int64_t>,
			{1, 2, 3},
			{-64, 15, -30000, 2}
			);
};

TEST(ObjectPack, pair) {
	typedef std::pair<float, std::string> TestPair;
	TEST_DATAPACK_IO(
			TestPair,
			{8.6f, "hello"},
			{9.5f, "world"}
			);
};

TEST(ObjectPack, unordered_map) {
	typedef std::unordered_map<std::uint32_t, std::string> TestMap;
	TEST_DATAPACK_IO(
			TestMap,
			{{1, "hello"}, {74, "world"}},
			{{0, "foo"}, {1, "bar"}, {122, "baz"}}
			);
}

TEST(ObjectPack, array) {
	typedef std::array<std::string, 3> TestArray;
	TEST_DATAPACK_IO(
			TestArray,
			{{"a", "b", "cdef"}},
			{{"foo", "bar", "baz"}}
			);
}

TEST(ObjectPack, empty_vec) {
	std::vector<int> vec;
	ObjectPack pack;
	pack.allocate(pack.size(vec));
	pack.put(vec);

	std::vector<int> read_vec = pack.get<std::vector<int>>();

	ASSERT_THAT(read_vec, IsEmpty());
}

TEST(ObjectPack, objectpack) {
	ObjectPack o1;
	o1.allocate(o1.size<double>());
	o1.put(18.7);

	ObjectPack o2;
	o2.allocate(o2.size<std::int32_t>());
	o2.put(-30434);

	ObjectPack o3;
	o3.allocate(o3.size<float>());
	o3.put(-2.4f);

	ObjectPack objectpack;
	std::string str = "hello world";
	objectpack.allocate(objectpack.size(str) + objectpack.size(o1) + objectpack.size(o2));
	objectpack.put(str);
	objectpack.put(o1);
	objectpack.put(o2);

	TEST_DATAPACK_IO(ObjectPack, objectpack, o3);
}

TEST(ObjectPack, expanded_objectpack) {
	ObjectPack o1;
	o1.allocate(8);
	o1.write((std::uint64_t) 0x0123456789ABCDEF);

	ObjectPack objectpack;
	objectpack.allocate(objectpack.size(o1));

	o1.expand(8);
	o1.write((std::uint64_t) 0xFEDCBA9876543210);
	objectpack.put(o1);

	o1 = objectpack.get<ObjectPack>();
	ASSERT_EQ(o1.get<std::uint64_t>(), (std::uint64_t) 0x0123456789ABCDEF);
	ASSERT_EQ(o1.get<std::uint64_t>(), (std::uint64_t) 0xFEDCBA9876543210);
}

TEST(ObjectPack, str) {
	std::string data = "hello world";
	ObjectPack serial_data(data);
	std::string deserial_data = serial_data.get<std::string>();

	ASSERT_EQ(data, deserial_data);
}

TEST(ObjectPack, str_vector) {
	std::vector<std::string> data = {"ab", "zzzzz", "678908DSDJSK"};
	ObjectPack serial_data(data);
	auto deserial_data = serial_data.get<std::vector<std::string>>();

	ASSERT_THAT(deserial_data, ElementsAreArray(data));
}

TEST(LightObjectPack, custom_rules) {
	NonDefaultConstructibleData data(8);

	// A custom light_json serialization rule is specified, so that nothing is
	// serialized...
	LightObjectPack pack = data;

	// ... and NonDefaultConstructibleData is initialized with 0
	NonDefaultConstructibleData unserialized_data = pack.get<NonDefaultConstructibleData>();
	ASSERT_EQ(unserialized_data.i, 0);
}

TEST(LightObjectPack, object_pack_fallback) {
	NonDefaultConstructibleSerializerOnly data(12);

	LightObjectPack pack = data;

	NonDefaultConstructibleData unserialized_data = pack.get<decltype(data)>();

	ASSERT_EQ(unserialized_data.i, 12);
}
