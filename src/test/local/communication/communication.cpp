#include "gtest/gtest.h"

#include "../mocks/communication/mock_communication.h"
#include "communication/communication.h"

using FPMAS::communication::TypedMpi;

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;
using ::testing::Pair;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::IsEmpty;


class MpiTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<3, 8> comm;

		void copy(
			std::unordered_map<int, std::string>& map,
			std::unordered_map<int, FPMAS::api::communication::DataPack>& dataPackMap
			) {
			for(auto item : map) {
				dataPackMap[item.first] = buildDataPack(item.second);
			}
		}

		FPMAS::api::communication::DataPack buildDataPack(std::string str) {
			FPMAS::api::communication::DataPack pack {(int) str.size(), sizeof(char)};
			std::memcpy(pack.buffer, str.data(), str.size());
			return pack;
		}
};

TEST_F(MpiTest, migrate_int) {
	TypedMpi<int> mpi {comm};
	std::unordered_map<int, std::vector<int>> exportMap =
	{ {1, {4, 8, 7}}, {6, {2, 3}}};

	auto exportMapMatcher = UnorderedElementsAre(
		Pair(1, AnyOf(
				buildDataPack("[4,8,7]"),
				buildDataPack("[4,7,8]"),
				buildDataPack("[8,4,7]"),
				buildDataPack("[8,7,4]"),
				buildDataPack("[7,8,4]"),
				buildDataPack("[7,4,8]"))),
		Pair(6, AnyOf(buildDataPack("[2,3]"), buildDataPack("[3,2]")))
		);

	std::unordered_map<int, std::string> importMap = {
		{0, "[3]"},
		{7, "[1,2,3]"}
	};

	std::unordered_map<int, FPMAS::api::communication::DataPack> importDataPack;
	copy(importMap, importDataPack);

	EXPECT_CALL(
		comm, allToAll(exportMapMatcher, MPI_CHAR))
		.WillOnce(Return(importDataPack));

	std::unordered_map<int, std::vector<int>> result
		= mpi.migrate(exportMap);

	ASSERT_THAT(result, UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(3)),
		Pair(7, UnorderedElementsAre(1, 2, 3))
		));
}

TEST_F(MpiTest, gather_int) {
	TypedMpi<int> mpi {comm};
	int export_int = 8;

	EXPECT_CALL(comm, gather(buildDataPack("8"), MPI_CHAR, 2));

	mpi.gather(export_int, 2);
}

TEST_F(MpiTest, gather_int_root) {
	TypedMpi<int> mpi {comm};
	int local_data = 2;

	// Normally, exactly one item is resized by proc (8 in this case), but not
	// important in the context of this test.
	std::vector<FPMAS::communication::DataPack> import = {
		buildDataPack("2"),
		buildDataPack("4"),
		buildDataPack("1"),
		buildDataPack("3")
	};

	EXPECT_CALL(comm, gather(buildDataPack("2"), MPI_CHAR, 4))
		.WillOnce(Return(import));

	std::vector<int> recv = mpi.gather(local_data, 4);

	ASSERT_THAT(recv, ElementsAre(2, 4, 1, 3));
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

TEST_F(MpiTest, migrate_fake_test) {
	TypedMpi<FakeType> mpi {comm};
	FakeType fake1 {2, "hello", 4.5};
	FakeType fake2 {-1, "world", .6};
	FakeType fake3 {0, "foo", 8};
	std::unordered_map<int, std::vector<FakeType>> exportMap = {
			{0, {fake1, fake2}},
			{3, {fake3}},
		};

	auto exportMapMatcher = UnorderedElementsAre(
		Pair(0, AnyOf(
				buildDataPack(nlohmann::json({fake1, fake2}).dump()),
				buildDataPack(nlohmann::json({fake2, fake1}).dump())
				)),
		Pair(3, buildDataPack(nlohmann::json({fake3}).dump()))
		);

	FakeType importFake1 {10, "import", 4.2};
	FakeType importFake2 {8, "import_2", 4.5};
	std::unordered_map<int, std::string> importMap =
	{{2, nlohmann::json({importFake1, importFake2}).dump()}};

	std::unordered_map<int, FPMAS::api::communication::DataPack> importDataPack;
	copy(importMap, importDataPack);

	EXPECT_CALL(comm, allToAll(exportMapMatcher, MPI_CHAR))
		.WillOnce(Return(importDataPack));

	auto result = mpi.migrate(exportMap);

	ASSERT_THAT(result, ElementsAre(
		Pair(2, UnorderedElementsAre(importFake1, importFake2))
		));
}
