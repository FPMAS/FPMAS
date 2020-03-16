#include "gtest/gtest.h"

#include <mpi.h>
#include <unordered_map>
#include <thread>
#include <chrono>

#include "communication/resource_container.h"
#include "communication/sync_communication.h"
#include "utils/test.h"

using namespace std::chrono_literals;

using FPMAS::communication::SyncMpiCommunicator;

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

		std::string getLocalData(unsigned long id) const override {
			return std::to_string(data.at(id));
		}

		std::string getUpdatedData(unsigned long id) const override {
			return std::to_string(data.at(id));
		}

		void updateData(unsigned long id, std::string data_str) override {
			this->data[id] = std::stoul(data_str);
		}
};

class Mpi_TestReadersWriters : public ::testing::Test {
	public:
		TestResourceHandler handler;
		SyncMpiCommunicator comm;

		Mpi_TestReadersWriters() : comm(handler) {}

};

/*
 * Simple test that perform multiple readings on proc 0
 */
TEST_F(Mpi_TestReadersWriters, test_concurrent_readings) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() >= 1) {
			std::string data = comm.read(0, 0);
			ASSERT_EQ(data, "0");
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(test_concurrent_readings, 2);
	}
}

/*
 * Test that when a local resource is acquired by an other proc, the local proc
 * properly wait that data is given back to read it itself.
 */
TEST_F(Mpi_TestReadersWriters, self_read_while_external_acquire) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() == 1) {
			// Acquires the resource from 0 before 0 itself reads it
			std::string data = comm.acquire(0, 0);
			ASSERT_EQ(data, "0");

			// Makes the read on 0 to delay
			std::this_thread::sleep_for(100ms);

			// Updates data
			this->handler.data[0] = 12;

			// Gives back resource to 0 so that it can read it
			comm.giveBack(0, 0);
		}
		else if (comm.getRank() == 0) {
			// Ensures that 1 has initiated acquire
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

/*
 * Test that reading from other proc is properly delayed while the local proc
 * has itself acquired the local resource.
 */
TEST_F(Mpi_TestReadersWriters, external_read_while_self_acquire) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() == 1) {
			// Ensures 0 has acquired its resource
			std::this_thread::sleep_for(50ms);

			auto start = std::chrono::system_clock::now();
			// Blocks until 0 gives back the resource to itself
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
			// Acquires the local resource before 1 reads it
			std::string data = comm.acquire(0, 0);
			ASSERT_EQ(data, "0");
			this->handler.data[0] = 0;

			// Waits for the read request from 1
			MPI_Status status;
			MPI_Probe(1, FPMAS::communication::Tag::READ, comm.getMpiComm(), &status);
			// Receives and handle the read, but the resource is not available
			comm.handleIncomingRequests();

			// Makes the read on 0 to delay
			std::this_thread::sleep_for(100ms);

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
 * In a way, readings are atomic in our implementation.
 * The only thing to assert is that the acquire is handled at some point, but
 * it does not matter when in the read requests processing.
 */
TEST_F(Mpi_TestReadersWriters, acquiring_while_read) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			for(int i = 0; i < 5; i++) {
				std::this_thread::sleep_for(10ms);
				std::string data = comm.read(0, 0);
			}
		}
		else if (comm.getRank() == 2) {
			std::this_thread::sleep_for(40ms);
			std::string data = comm.acquire(0, 0);
			ASSERT_EQ(data, "0");
			this->handler.data[0] = 12;
			comm.giveBack(0, 0);
		}
		comm.terminate();
		if(comm.getRank() == 0) {
			ASSERT_EQ(this->handler.data[0], 12);
		}
	}
	else {
		PRINT_MIN_PROCS_WARNING(test_acquiring_while_read, 3);
	}
}

/*
 * In this test, all procs (except 0 itself) are concurrently accessing data on
 * 0. Proc 1 acquires it first, and all others try to read it.
 * The test asserts that read request are delayed until proc 1 gives back the
 * resource to 0.
 */
TEST_F(Mpi_TestReadersWriters, reading_while_acquired) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 0) {
			// Does nothing, just terminates and handles requests
		}
		else if(comm.getRank() == 1) {
			// Acquires data befor 2 tries to read it
			std::string data = comm.acquire(0, 0);
			this->handler.data[0] = 12;

			// Holds the resource for at least 200ms
			std::this_thread::sleep_for(200ms);

			comm.giveBack(0, 0);
		}
		// All other procs are trying to read the data acquired by 1
		else if (comm.getRank() >= 2) {
			// Ensures that the read is performed while data is acquired by 1
			std::this_thread::sleep_for(40ms);
			std::string data = comm.read(0, 0);

			// Proof that the read has wait until data was given back by 1
			ASSERT_EQ(data, "12");
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(reading_while_acquired, 3);
	}
}

/*
 * In this test, all procs (except 0 itself) are concurrently acquiring a
 * resource on 0. The test asserts that the race condition is properly handled,
 * ie each proc has an exclusive write access.
 */
TEST_F(Mpi_TestReadersWriters, concurrent_acquiring) {
	if(comm.getSize() >= 2) {
		if(comm.getRank() == 0) {
			auto start = std::chrono::system_clock::now();
			// Does nothing and handles requests
			comm.terminate();
			auto end = std::chrono::system_clock::now();

			// Because each other proc takes at least 10ms to process data,
			// this proc should keep handling request for at least 10*(N-1)ms
			std::chrono::duration<double, std::milli> duration = end - start;
			std::chrono::duration<double, std::milli> min (10 * (comm.getSize() - 1));
			ASSERT_GE(duration, min);

			// Ensures that the data has well been exclusively written by only
			// one proc at a time
			ASSERT_EQ(this->handler.data[0], comm.getSize() - 1);
		}
		else {
			// Acquires the resource from 0
			std::string data = comm.acquire(0, 0);
			// Adds 1 to the current value on proc 0
			this->handler.data[0] = std::stoul(data) + 1;

			// Waits before give data back
			std::this_thread::sleep_for(10ms);
			comm.giveBack(0, 0);

			comm.terminate();
		}
	}
	else {
		PRINT_MIN_PROCS_WARNING(concurrent_acquiring, 2);
	}
}
