#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include "gtest/gtest.h"
#include "test_utils/test_utils.h"
#include "communication/communication.h"
#include "communication/resource_manager.h"

using FPMAS::communication::ResourceManager;
using FPMAS::communication::MpiCommunicator;
using FPMAS::communication::TerminableMpiCommunicator;

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

class TestResourceHandler : public ResourceManager {
	public:
		std::chrono::milliseconds delay;
		TestResourceHandler() : delay(0) {
		}

		std::string getResource(unsigned long id) {
			std::this_thread::sleep_for(this->delay);
			return std::to_string(id);
		}

};

class Mpi_TerminationTest : public ::testing::Test {
	protected:
		TestResourceHandler handler;

};

TEST_F(Mpi_TerminationTest, simple_termination_test) {
	TerminableMpiCommunicator comm(&handler);

	comm.terminate();

}


TEST_F(Mpi_TerminationTest, termination_test_with_delay) {
	TerminableMpiCommunicator comm(&handler);

	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		std::this_thread::sleep_for(1s);
	}

	comm.terminate();
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);

}

TEST_F(Mpi_TerminationTest, read_from_passive_procs_test) {
	TerminableMpiCommunicator comm(&handler);

	if(comm.getSize() >= 2) {
		if(comm.getRank() % 2 == 0 && comm.getRank() != comm.getSize() - 1) {
			// Ensures that even procs enter passive mode
			std::this_thread::sleep_for(100ms);

			std::string data = comm.read(comm.getRank(), comm.getRank() + 1);
			ASSERT_EQ(data, std::to_string(comm.getRank()));
		}
		comm.terminate();
	} else {
		PRINT_MIN_PROCS_WARNING(read_from_passive_procs_test, 2);
	}
}

TEST_F(Mpi_TerminationTest, read_from_active_procs_test) {
	TerminableMpiCommunicator comm(&handler);

	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Delays the request reception
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Will stay active for 500 ms due to proc 1 reception delay
			// By this time, proc 2 should be able to get its response from
			// this proc
			std::string data = comm.read(0, 1);
			ASSERT_EQ(data, "0");
		}
		else if (comm.getRank() == 2) {
			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = comm.read(2, 0);
			ASSERT_EQ(data, "2");
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(read_from_passive_procs_test, 3);
	}

}
