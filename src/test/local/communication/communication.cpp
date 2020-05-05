#include "gtest/gtest.h"

#include "../mocks/communication/mock_communication.h"
#include "communication/communication.h"

using FPMAS::communication::Mpi;

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;
using ::testing::Pair;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::IsEmpty;



class MigrateTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<3, 8> comm;
};

TEST_F(MigrateTest, migrate_int) {
	Mpi<int> mpi {comm};
	std::unordered_map<int, std::vector<int>> exportMap =
	{ {1, {4, 8, 7}}, {6, {2, 3}}};
	auto exportMapMatcher = UnorderedElementsAre(
		Pair(1, AnyOf("[4,8,7]", "[4,7,8]", "[8,4,7]", "[8,7,4]","[7,8,4]","[7,4,8]")),
		Pair(6, AnyOf("[2,3]", "[3,2]"))
		);

	std::unordered_map<int, std::string> importMap = {
		{0, "[3]"},
		{7, "[1,2,3]"}
	};

	EXPECT_CALL(
		comm, allToAll(exportMapMatcher))
		.WillOnce(Return(importMap));

	std::unordered_map<int, std::vector<int>> result
		= mpi.migrate(exportMap);

	ASSERT_THAT(result, UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(3)),
		Pair(7, UnorderedElementsAre(1, 2, 3))
		));
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

TEST_F(MigrateTest, migrate_fake_test) {
	Mpi<FakeType> mpi {comm};
	FakeType fake1 {2, "hello", 4.5};
	FakeType fake2 {-1, "world", .6};
	FakeType fake3 {0, "foo", 8};
	std::unordered_map<int, std::vector<FakeType>> exportMap = {
			{0, {fake1, fake2}},
			{3, {fake3}},
		};

	auto exportMapMatcher = UnorderedElementsAre(
		Pair(0, AnyOf(
				nlohmann::json({fake1, fake2}).dump(),
				nlohmann::json({fake2, fake1}).dump()
				)),
		Pair(3, nlohmann::json({fake3}).dump())
		);

	FakeType importFake1 {10, "import", 4.2};
	FakeType importFake2 {8, "import_2", 4.5};
	std::unordered_map<int, std::string> importMap =
	{{2, nlohmann::json({importFake1, importFake2}).dump()}};

	EXPECT_CALL(comm, allToAll(exportMapMatcher))
		.WillOnce(Return(importMap));

	auto result = mpi.migrate(exportMap);

	ASSERT_THAT(result, ElementsAre(
		Pair(2, UnorderedElementsAre(importFake1, importFake2))
		));
}
