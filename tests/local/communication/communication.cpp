#include "gtest/gtest.h"

#include "communication/mock_communication.h"
#include "fpmas/communication/communication.h"

using fpmas::communication::TypedMpi;

using namespace testing;

class MpiTest : public Test {
	protected:
		MockMpiCommunicator<3, 8> comm;
};

template<typename T>
std::vector<T> deserialize_vec(const fpmas::communication::DataPack& pack) {
	return TypedMpi<T>::pack_type::parse(pack).template get<std::vector<T>>();
}
template<typename T>
fpmas::communication::DataPack serialize_vec(std::vector<T> vec) {
	return typename TypedMpi<T>::pack_type(vec).dump();
}

TEST_F(MpiTest, migrate_int) {
	TypedMpi<int> mpi {comm};
	std::unordered_map<int, std::vector<int>> exportMap =
	{ {1, {4, 8, 7}}, {6, {2, 3}}};

	auto export_map_matcher = UnorderedElementsAre(
		Pair(1, ResultOf(&deserialize_vec<int>, UnorderedElementsAre(4, 8, 7))),
		Pair(6, ResultOf(&deserialize_vec<int>, UnorderedElementsAre(2, 3)))
		);

	std::unordered_map<int, fpmas::communication::DataPack> import_map = {
		{0, serialize_vec<int>({3})},
		{7, serialize_vec<int>({1,2,3})}
	};

	EXPECT_CALL(
		comm, allToAll(export_map_matcher, MPI_CHAR))
		.WillOnce(Return(import_map));

	std::unordered_map<int, std::vector<int>> result
		= mpi.migrate(exportMap);

	ASSERT_THAT(result, UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(3)),
		Pair(7, UnorderedElementsAre(1, 2, 3))
		));
}

template<typename T, typename PackType = TypedMpi<int>::pack_type>
T deserialize(const fpmas::communication::DataPack& pack) {
	return PackType::parse(pack).template get<T>();
}
template<typename T, typename PackType = TypedMpi<int>::pack_type>
fpmas::communication::DataPack serialize(T item) {
	return PackType(item).dump();
}

TEST_F(MpiTest, gather_int) {
	TypedMpi<int> mpi {comm};
	int export_int = 8;

	EXPECT_CALL(comm, gather(ResultOf(&deserialize<int>, 8), MPI_CHAR, 2));

	mpi.gather(export_int, 2);
}

TEST_F(MpiTest, gather_int_root) {
	TypedMpi<int> mpi {comm};
	int local_data = 2;

	// Normally, exactly one item is received by proc (8 in this case), but not
	// important in the context of this test.
	std::vector<fpmas::communication::DataPack> import = {
		serialize(0),
		serialize(4),
		serialize(2),
		serialize(3)
	};

	EXPECT_CALL(comm, gather(ResultOf(&deserialize<int>, local_data), MPI_CHAR, 2))
		.WillOnce(Return(import));

	std::vector<int> recv = mpi.gather(local_data, 2);

	ASSERT_THAT(recv, ElementsAre(0, 4, 2, 3));
}

TEST_F(MpiTest, all_gather) {
	TypedMpi<int> mpi {comm};
	int local_data = 2;

	// Normally, exactly one item is received by proc (8 in this case), but not
	// important in the context of this test.
	std::vector<fpmas::communication::DataPack> import = {
		serialize(2),
		serialize(4),
		serialize(1),
		serialize(3)
	};

	EXPECT_CALL(comm, allGather(ResultOf(&deserialize<int>, local_data), MPI_CHAR))
		.WillOnce(Return(import));

	std::vector<int> recv = mpi.allGather(local_data);

	ASSERT_THAT(recv, ElementsAre(2, 4, 1, 3));
}

TEST_F(MpiTest, all_to_all_int) {
	TypedMpi<int> mpi {comm};

	std::unordered_map<int, int> export_map {
		{0, 0}, {2, -2}, {3, 4}};
	auto export_map_matcher = UnorderedElementsAre(
			Pair(0, ResultOf(&deserialize<int>, 0)),
			Pair(2, ResultOf(&deserialize<int>, -2)),
			Pair(3, ResultOf(&deserialize<int>, 4))
			);
	std::unordered_map<int, fpmas::api::communication::DataPack> import_map {
		{0, serialize(4)}, {2, serialize(-13)}};

	EXPECT_CALL(comm, allToAll(export_map_matcher, MPI_CHAR))
		.WillOnce(Return(import_map));

	auto recv = mpi.allToAll(export_map);

	ASSERT_THAT(recv, UnorderedElementsAre(Pair(0, 4), Pair(2, -13)));
}

TEST_F(MpiTest, bcast_int) {
	TypedMpi<int> mpi {comm};
	int export_int = 8;

	auto recv = serialize(8);
	EXPECT_CALL(comm, bcast(ResultOf(&deserialize<int>, export_int), MPI_CHAR, 2))
		.WillOnce(Return(recv));

	ASSERT_EQ(mpi.bcast(export_int, 2), export_int);
}

struct FakeType {
	int field;
	std::string field2;
	double field3;
	FakeType() {};
	FakeType(int field, std::string field2, double field3)
		: field(field), field2(field2), field3(field3) {}

	bool operator==(const FakeType& other) const {
		return
			(this->field == other.field)
			&& (this->field2 == other.field2)
			&& (this->field3 == other.field3);
	}
};

void to_json(nlohmann::json& j, const FakeType& o) {
	j = nlohmann::json{{"f", o.field}, {"f2", o.field2}, {"f3", o.field3}};
}

void from_json(const nlohmann::json& j, FakeType& o) {
	j.at("f").get_to(o.field);
	j.at("f2").get_to(o.field2);
	j.at("f3").get_to(o.field3);
}

namespace fpmas { namespace io { namespace datapack {
	template<>
	struct Serializer<FakeType> {
		static void to_datapack(ObjectPack& pack, const FakeType& fake) {
			std::size_t size =
				pack_size<int>() + pack_size(fake.field2) + pack_size<double>();
			pack.allocate(size);
			pack.write(fake.field);
			pack.write(fake.field2);
			pack.write(fake.field3);
		}

		static FakeType from_datapack(const ObjectPack& pack) {
			FakeType fake;
			pack.read(fake.field);
			pack.read(fake.field2);
			pack.read(fake.field3);

			return fake;
		}
	};
}}}

template<typename Mpi>
void test_all_to_all(
		MockMpiCommunicator<3, 8>& comm,
		Mpi& mpi) {
	FakeType fake1 {2, "hello", 4.5};
	FakeType fake2 {-1, "world", .6};
	FakeType fake3 {0, "foo", 8};
	std::unordered_map<int, FakeType> export_map = {
			{0, fake1},
			{2, fake2},
			{3, fake3},
		};

	auto export_map_matcher = UnorderedElementsAre(
		Pair(0, ResultOf(&deserialize<FakeType, typename Mpi::pack_type>, fake1)),
		Pair(2, ResultOf(&deserialize<FakeType, typename Mpi::pack_type>, fake2)),
		Pair(3, ResultOf(&deserialize<FakeType, typename Mpi::pack_type>, fake3))
		);

	FakeType import_fake_1 {10, "import", 4.2};
	FakeType import_fake_2 {8, "import_2", 4.5};
	std::unordered_map<int, fpmas::io::datapack::DataPack> import_map = {
		{2, serialize<FakeType, typename Mpi::pack_type>(import_fake_1)},
		{6, serialize<FakeType, typename Mpi::pack_type>(import_fake_2)}
	};

/*
 *    EXPECT_CALL(comm, allToAll(export_map_matcher, MPI_CHAR))
 *        .WillOnce(Return(import_map));
 *
 *    auto result = mpi.allToAll(export_map);
 *
 *    ASSERT_THAT(result, UnorderedElementsAre(
 *        Pair(2, import_fake_1),
 *        Pair(6, import_fake_2)
 *        ));
 */
}

TEST_F(MpiTest, all_to_all_fake_test_json) {
	fpmas::communication::detail::TypedMpi<FakeType, fpmas::io::datapack::JsonPack> mpi {comm};
	test_all_to_all(comm, mpi);
}

TEST_F(MpiTest, all_to_all_fake_test_datapack) {
	fpmas::communication::detail::TypedMpi<FakeType, fpmas::io::datapack::ObjectPack> mpi {comm};
	test_all_to_all(comm, mpi);
}

TEST_F(MpiTest, migrate_fake_test) {
	TypedMpi<FakeType> mpi {comm};
	FakeType fake1 {2, "hello", 4.5};
	FakeType fake2 {-1, "world", .6};
	FakeType fake3 {0, "foo", 8};
	std::unordered_map<int, std::vector<FakeType>> export_map = {
			{0, {fake1, fake2}},
			{3, {fake3}},
		};

	auto export_map_matcher = UnorderedElementsAre(
		Pair(0, ResultOf(&deserialize_vec<FakeType>, UnorderedElementsAre(fake1, fake2))),
		Pair(3, ResultOf(&deserialize_vec<FakeType>, ElementsAre(fake3)))
		);

	FakeType import_fake_1 {10, "import", 4.2};
	FakeType import_fake_2 {8, "import_2", 4.5};
	std::unordered_map<int, fpmas::io::datapack::DataPack> import_map =
	{{2, serialize_vec<FakeType>({import_fake_1, import_fake_2})}};

	EXPECT_CALL(comm, allToAll(export_map_matcher, MPI_CHAR))
		.WillOnce(Return(import_map));

	auto result = mpi.migrate(export_map);

	ASSERT_THAT(result, ElementsAre(
		Pair(2, UnorderedElementsAre(import_fake_1, import_fake_2))
		));
}
