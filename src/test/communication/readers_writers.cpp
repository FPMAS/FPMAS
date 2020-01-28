#include "gtest/gtest.h"

#include <mpi.h>
#include <unordered_map>
#include <thread>
#include <chrono>

#include "communication/resource_container.h"
#include "communication/communication.h"
#include "test_utils/test_utils.h"

using namespace std::chrono_literals;

using FPMAS::communication::TerminableMpiCommunicator;

class TestResourceHandler : public FPMAS::communication::ResourceContainer {
	public:
		std::unordered_map<unsigned long, unsigned long> data;

		TestResourceHandler() {
			int current_rank;
			MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
			int size;
			MPI_Comm_size(MPI_COMM_WORLD, &size);
			for (int i = 0; i < size; i++)
					data[i] = i;
		}

		std::string getData(unsigned long id) const override {
			return std::to_string(data.at(id));
		}

		void updateData(unsigned long id, std::string data_str) override {
			this->data[id] = std::stoul(data_str);
		}
};

class Mpi_TestReadersWriters : public ::testing::Test {
	public:
		TestResourceHandler handler;
		TerminableMpiCommunicator comm;

		Mpi_TestReadersWriters() : comm(handler) {}

};

TEST_F(Mpi_TestReadersWriters, test_concurrent_readings) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			std::string data = comm.read(0, 0);
			ASSERT_EQ(data, "0");
		}
		else if (comm.getRank() == 0) {

		}
		else if (comm.getRank() == 2) {
			std::string data = comm.read(0, 0);
			ASSERT_EQ(data, "0");
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(test_concurrent_readings, 3);
	}
}

TEST_F(Mpi_TestReadersWriters, self_read_while_external_acquire) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() == 1) {

			// Acquires the resource from 0 before 0 itself reads it
			std::string data = comm.acquire(0, 0);
			ASSERT_EQ(data, "0");
			this->handler.data[0] = 0;

			// Makes the read on 0 to delay
			std::this_thread::sleep_for(100ms);

			// Updates data
			this->handler.data[0] = 12;

			// Gives back resource to 0 so that it can read it
			comm.giveBack(0, 0);
		}
		else if (comm.getRank() == 0) {
			// Ensures 1 has initiated acquire
			std::this_thread::sleep_for(50ms);

			auto start = std::chrono::system_clock::now();
			// Receives and handles the acquire from 1
			comm.handleIncomingRequests();

			// Now, try to read the local resource acquired by 1 : 
			// response should be delayed by at least 100ms, waiting for 1 to
			// give back the resource.
			std::string data = comm.read(0, 0);

			auto end = std::chrono::system_clock::now();

			std::chrono::duration<double, std::milli> duration = end - start;
			std::chrono::duration<double, std::milli> min (100);
			// Assert that the response have been delayed
			ASSERT_GE(duration, min);

			// Assert that the data returned is the one updated by the previous
			// acquire
			ASSERT_EQ(data, "12");
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(self_read_while_external_acquire, 2);
	}
}

TEST_F(Mpi_TestReadersWriters, external_read_while_self_acquire) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() == 1) {
			std::this_thread::sleep_for(1000ms);
			auto start = std::chrono::system_clock::now();
			std::cout << "send read req" << std::endl;
			// Acquires the resource from 0 before 0 itself reads it
			std::string data = comm.read(0, 0);

			auto end = std::chrono::system_clock::now();

			std::chrono::duration<double, std::milli> duration = end - start;
			std::chrono::duration<double, std::milli> min (100);
			// Assert that the response have been delayed
			ASSERT_GE(duration, min);

			// Assert that the data returned is the one updated by the previous
			// acquire
			ASSERT_EQ(data, "12");
		}
		else if (comm.getRank() == 0) {
			std::string data = comm.acquire(0, 0);
			std::this_thread::sleep_for(2000ms);
			ASSERT_EQ(data, "0");
			this->handler.data[0] = 0;

			MPI_Status status;
			MPI_Probe(1, FPMAS::communication::Tag::READ, comm.getMpiComm(), &status);
			std::cout << "[0] handle incoming" << std::endl;
			// Receives the read, but the resource is not available
			comm.handleIncomingRequests();

			std::cout << "start sleep" << std::endl;

			// Makes the read on 0 to delay
			std::this_thread::sleep_for(3000ms);

			// Updates data
			this->handler.data[0] = 12;

			// Gives back resource to itself so that 1 can read it
			comm.giveBack(0, 0);
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(self_read_while_external_acquire, 2);
	}
}
/*
 *TEST_F(Mpi_TestReadersWriters, test_acquiring_while_reading) {
 *    if(comm.getSize() >= 3) {
 *        if(comm.getRank() == 1) {
 *            std::string data = comm.read(0, 0);
 *            ASSERT_EQ(data, "0");
 *        }
 *        else if (comm.getRank() == 0) {
 *
 *        }
 *        else if (comm.getRank() == 2) {
 *            std::string data = comm.read(0, 0);
 *            ASSERT_EQ(data, "0");
 *        }
 *        comm.terminate();
 *    }
 *    else {
 *        PRINT_MIN_PROCS_WARNING(test_concurrent_readings, 3);
 *    }
 *}
 */
