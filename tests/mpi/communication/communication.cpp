#include "fpmas/communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "utils/test.h"
#include "fpmas/utils/macros.h"
#include <chrono>
#include <thread>

using ::testing::Contains;
using ::testing::IsEmpty;

using fpmas::communication::MpiCommunicator;
using fpmas::communication::TypedMpi;

TEST(MpiCommunicatorTest, size_test) {
	MpiCommunicator comm;

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	ASSERT_EQ(size, comm.getSize());
}

TEST(MpiCommunicatorTest, rank_test) {
	MpiCommunicator comm;

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	ASSERT_EQ(rank, comm.getRank());
}

TEST(MpiCommunicatorTest, probe_any_source) {
	MpiCommunicator comm;

	FPMAS_MIN_PROCS("MpiCommunicatorTest.probe_any_source", comm, 2) {
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
				comm.recv(NULL, 0, MPI_INT, status.MPI_SOURCE, 4, MPI_STATUS_IGNORE);
			}
			ASSERT_THAT(ranks, IsEmpty());
		}
		else {
			comm.send(0, 4);
		}
	}
}

TEST(MpiMigrationTest, simple_migration_test) {
	MpiCommunicator comm;
	TypedMpi<int> mpi {comm};
	
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
TEST(MpiMigrationTest, variable_recv_size_migration) {
	MpiCommunicator comm;
	TypedMpi<int> mpi {comm};

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
TEST(MpiMigrationTest, variable_send_size_migration) {
	MpiCommunicator comm;
	TypedMpi<int> mpi {comm};

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

TEST(MpiGatherTest, gather) {
	MpiCommunicator comm;
	TypedMpi<int> mpi {comm};

	int local_data = (comm.getRank()+1) * 10;
	std::vector<int> data = mpi.gather(local_data, comm.getSize() - 1);

	if(comm.getRank() != comm.getSize() - 1) {
		ASSERT_THAT(data, IsEmpty());
	} else {
		int sum = 0;
		for(auto i : data)
			sum+=i;
		ASSERT_EQ(sum, 10*(comm.getSize() * (comm.getSize() + 1)/2));
	}
}

TEST(MpiIssendTest, edge_case) {
	MpiCommunicator comm;
	std::string data;
	// Generates a long string
	for(int i = 0; i < 10000; i++) {
		data += std::to_string(i);
	}
	FPMAS_MIN_PROCS("MpiIssendTest.edge_case", comm, 2) {
		FPMAS_ON_PROC(comm, 0) {
			std::vector<MPI_Request> requests;
			for(int i = 1; i < comm.getSize(); i++) {
				std::string local_data = data;
				MPI_Request req;
				comm.Issend(local_data.c_str(), local_data.size(), MPI_CHAR, i, 0, &req);
				requests.push_back(req);
			}
			for(auto& req : requests)
				while(!comm.test(&req)){}
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(2));
			MPI_Status message_to_receive_status;
			comm.probe(0, 0, &message_to_receive_status);
			int count;
			MPI_Get_count(&message_to_receive_status, MPI_CHAR, &count);
			char* buffer = (char*) std::malloc(count * sizeof(char));

			MPI_Status status;
			comm.recv(buffer, count, MPI_CHAR, 0, 0, &status);

			std::string recv(buffer, count);
			ASSERT_EQ(data.size(), recv.size());
			ASSERT_EQ(data, recv);

			std::free(buffer);
		}
	}
}
