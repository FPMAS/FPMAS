#include "gtest/gtest.h"
#include "utils/test.h"
#include "api/communication/mock_communication.h"
#include "communication/communication.h"

using FPMAS::communication::MpiCommunicator;
using FPMAS::communication::Mpi;

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;
using ::testing::Pair;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::IsEmpty;

TEST(Mpi_MpiCommunicatorTest, size_test) {
	MpiCommunicator comm;

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	ASSERT_EQ(size, comm.getSize());
}

TEST(Mpi_MpiCommunicatorTest, rank_test) {
	MpiCommunicator comm;

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	ASSERT_EQ(rank, comm.getRank());
}

TEST(Mpi_MpiCommunicatorTest, build_from_ranks_test) {
	int global_size;
	MPI_Comm_size(MPI_COMM_WORLD, &global_size);

	if(global_size == 1) {
		MpiCommunicator comm {0};
		int comm_size;
		MPI_Comm_size(comm.getMpiComm(), &comm_size);
		ASSERT_EQ(comm.getSize(), 1);
		ASSERT_EQ(comm.getSize(), comm_size);

		int comm_rank;
		MPI_Comm_rank(comm.getMpiComm(), &comm_rank);
		ASSERT_EQ(comm.getRank(), 0);
		ASSERT_EQ(comm.getRank(), comm_rank);
	}
	else if(global_size >= 2) {
		int current_rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);

		if(!(global_size % 2 == 1 && current_rank == (global_size-1))) {
			MpiCommunicator* comm;
			if(current_rank % 2 == 0) {
				comm = new MpiCommunicator {current_rank, (current_rank + 1) % global_size};
			}
			else {
				comm = new MpiCommunicator {(current_rank - 1) % global_size, current_rank};
			}

			int comm_size;
			MPI_Comm_size(comm->getMpiComm(), &comm_size);
			ASSERT_EQ(comm->getSize(), 2);
			ASSERT_EQ(comm->getSize(), comm_size);

			int comm_rank;
			MPI_Comm_rank(comm->getMpiComm(), &comm_rank);
			ASSERT_EQ(comm->getRank(), comm_rank);
			delete comm;
		}
		else {
			// MPI requirement : all processes must call MPI_Comm_create
			MpiCommunicator comm {global_size - 1};
		}
	}

}

TEST(Mpi_MpiCommunicatorTest, probe_any_source) {
	MpiCommunicator comm;
	if(comm.getSize() > 1) {
		if(comm.getRank() == 0) {
			std::vector<int> ranks;
			for(int i = 1; i < comm.getSize(); i++) {
				ranks.push_back(i);
			}
			MPI_Status status;
			for(int i = 1; i < comm.getSize(); i++) {
				bool recv = comm.Iprobe(MPI_ANY_SOURCE, 4, &status);
				while(!recv) {
					recv = comm.Iprobe(MPI_ANY_SOURCE, 4, &status);
				}
				ASSERT_EQ(status.MPI_TAG, 4);
				ASSERT_THAT(ranks, Contains(status.MPI_SOURCE));
				ranks.erase(std::remove(ranks.begin(), ranks.end(), status.MPI_SOURCE));
				std::string data;
				comm.recv(&status, data);
			}
			ASSERT_THAT(ranks, IsEmpty());
		}
		else {
			comm.send("123", 0, 4);
		}
	}
}

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

TEST(Mpi_MigrationTest, simple_migration_test) {
	MpiCommunicator comm;
	Mpi<int> mpi {comm};
	
	std::unordered_map<int, std::vector<int>> export_map;
	for (int i = 0; i < comm.getSize(); i++) {
		export_map[i].push_back(comm.getRank());
	}

	auto import_map = mpi.migrate(export_map);
	ASSERT_EQ(import_map.size(), comm.getSize());
	for(auto item : import_map) {
		ASSERT_EQ(item.second.size(), 1);
		ASSERT_EQ(item.first, item.second[0]);
	}

}

// Each proc :
//   - sends the same amount of data to all others
//   - receives different amount of data from each proc
TEST(Mpi_MigrationTest, variable_recv_size_migration) {
	MpiCommunicator comm;
	Mpi<int> mpi {comm};

	std::unordered_map<int, std::vector<int>> export_map;
	for(int i = 0; i < comm.getSize(); i++) {
		for(int j = 0; j < comm.getRank(); j++) {
			export_map[i].push_back(j);
		}
	}
	auto import_map = mpi.migrate(export_map);

	ASSERT_EQ(import_map.size(), comm.getSize() - 1); // proc 0 doesn't send anything
	for(auto item : import_map) {
		ASSERT_EQ(item.second.size(), item.first);
		for(int j = 0; j < item.first; j++) {
			ASSERT_EQ(item.second.at(j), j);
		}

	}
}

// Each proc: 
//   - sends different amount of data to each proc
//   - receives the same amount of data from other
TEST(Mpi_MigrationTest, variable_send_size_migration) {
	MpiCommunicator comm;
	Mpi<int> mpi {comm};

	std::unordered_map<int, std::vector<int>> export_map;
	for(int i = 0; i < comm.getSize(); i++) {
		for(int j = 0; j < i; j++) {
			export_map[i].push_back(j);
		}
	}
	auto import_map = mpi.migrate(export_map);

	if(comm.getRank() == 0) {
		ASSERT_EQ(import_map.size(), 0); // proc 0 doesn't receive anything
	} else {
		ASSERT_EQ(import_map.size(), comm.getSize());
	}
	for(auto item : import_map) {
		ASSERT_EQ(item.second.size(), comm.getRank());
		for(int j = 0; j < comm.getRank(); j++) {
			ASSERT_EQ(item.second.at(j), j);
		}

	}


}
