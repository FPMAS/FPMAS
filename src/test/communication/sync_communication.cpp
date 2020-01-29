#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include "gtest/gtest.h"
#include "test_utils/test_utils.h"
#include "communication/sync_communication.h"
#include "communication/resource_container.h"

using FPMAS::communication::SyncMpiCommunicator;
using FPMAS::communication::ResourceContainer;

class TestResourceHandler : public ResourceContainer {
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

class Mpi_TerminationTest : public ::testing::Test {
	protected:
		TestResourceHandler handler;
		SyncMpiCommunicator comm;

		Mpi_TerminationTest() : comm(handler) {}

};

TEST_F(Mpi_TerminationTest, simple_termination_test) {
	comm.terminate();
}


TEST_F(Mpi_TerminationTest, termination_test_with_delay) {

	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		std::this_thread::sleep_for(1s);
	}

	comm.terminate();
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);

}

TEST_F(Mpi_TerminationTest, self_read) {
	std::string data = comm.read(comm.getRank(), comm.getRank());
	ASSERT_EQ(data, std::to_string(comm.getRank()));

	comm.terminate();
}

TEST_F(Mpi_TerminationTest, self_acquire) {
	std::string data = comm.acquire(comm.getRank(), comm.getRank());
	this->handler.data[comm.getRank()] = std::stoul(data);
	this->handler.data[comm.getRank()] = 10;
	comm.giveBack(comm.getRank(), comm.getRank());

	ASSERT_EQ(this->handler.data[comm.getRank()], 10);

	comm.terminate();
}

/**
 * The following test serie :
 * - read_from_passive_procs_test
 * - read_from_reading_procs_test
 * - read_from_acquiring_procs_test
 * - acquire_from_passive_procs_test
 * - acquire_from_reading_procs_test
 * - acquire_from_acquiring_procs_test
 *
 * ensures that procs are still able to handle request while they are
 * themselves performing a request or waiting for the end of execution,
 * preventing any deadlock.
 *
 * Notice that even if multiple request are performed simultaneously, no
 * concurrency access is assumed is those examples (ie multiple procs trying to
 * access the same ressource). This is tested in the readers_writers.cpp test serie.
 */

/**
 * Tests if its possible to read a data from a passive proc (ie a proc waiting
 * for termination after terminate() has been called.
 */
TEST_F(Mpi_TerminationTest, read_from_passive_procs_test) {
	if(comm.getSize() >= 2) {
		// Each even proc asks for data to proc + 1 (if it exists)
		if(comm.getRank() % 2 == 0 && comm.getRank() != comm.getSize() - 1) {
			// Ensures that even procs enter passive mode
			std::this_thread::sleep_for(100ms);

			std::string data = comm.read(comm.getRank(), comm.getRank() + 1);
			ASSERT_EQ(data, std::to_string(comm.getRank()));
		}
		// Odd procs immediatly enter passive mode
		comm.terminate();
	} else {
		PRINT_MIN_PROCS_WARNING(read_from_passive_procs_test, 2);
	}
}

/**
 * Tests if its possible to read a data from an active proc performing an
 * acquisition (ie a proc waiting for a response after acquie() has been called)
 */
TEST_F(Mpi_TerminationTest, read_from_acquiring_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Delays the acquire request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Will stay active for 500 ms due to proc 1 reception delay
			// By this time, proc 2 should be able to get its response from
			// this proc
			std::string data = comm.acquire(0, 1);
			ASSERT_EQ(data, "0");

			comm.giveBack(0, 1);
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
		PRINT_MIN_PROCS_WARNING(read_from_acquiring_procs_test, 3);
	}

}

/**
 * Tests if its possible to read a data from an active proc performing a
 * reading (ie a proc waiting for a response after read() has been called)
 */
TEST_F(Mpi_TerminationTest, read_from_reading_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Delays the read request reception from 0
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
		PRINT_MIN_PROCS_WARNING(read_from_reading_procs_test, 3);
	}

}

/**
 * Tests if its possible to acquire a data from a passive proc (ie a proc waiting
 * for termination after terminate() has been called.
 */
TEST_F(Mpi_TerminationTest, acquire_from_passive_procs_test) {
	if(comm.getSize() >= 2) {
		// Each even proc asks for data to proc + 1 (if it exists)
		if(comm.getRank() % 2 == 0 && comm.getRank() != comm.getSize() - 1) {
			// Ensures that even procs enter passive mode
			std::this_thread::sleep_for(100ms);

			std::string data_str = comm.acquire(comm.getRank(), comm.getRank() + 1);
			ASSERT_EQ(data_str, std::to_string(comm.getRank()));

			this->handler.data[comm.getRank()] = std::stoul(data_str);
			this->handler.data[comm.getRank()] = comm.getRank() + 1;

			comm.giveBack(comm.getRank(), comm.getRank() + 1);
		}
		// Odd procs immediatly enter passive mode
		comm.terminate();
		if(comm.getRank() % 2 == 1)
			ASSERT_EQ(this->handler.data.at(comm.getRank() - 1), comm.getRank());

	} else {
		PRINT_MIN_PROCS_WARNING(acquire_from_passive_procs_test, 2);
	}
}

/**
 * Tests if it's possible to acquire a data from an active proc performing a
 * reading (ie a proc waiting for a response after read() has been called)
 */
TEST_F(Mpi_TerminationTest, acquire_from_reading_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Delays the read request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Will stay active, waiting for the reading request, for 500 ms
			// due to proc 1 reception delay By this time, proc 2 should be
			// able to get its response from this proc
			std::string data = comm.read(0, 1);
			ASSERT_EQ(data, "0");

			ASSERT_EQ(this->handler.data[2], 10);
		}
		else if (comm.getRank() == 2) {
			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = comm.acquire(2, 0);
			ASSERT_EQ(data, "2");

			this->handler.data[2] = std::stoul(data);
			this->handler.data[2] = 10;

			comm.giveBack(2, 0);
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(acquire_from_reading_procs_test, 3);
	}

}

/**
 * Tests if it's possible to acquire a data from an active proc performing an
 * acquisition (ie a proc waiting for a response after acquire() has been called)
 */
TEST_F(Mpi_TerminationTest, acquire_from_acquiring_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Delays the acquire request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Will stay active, waiting for the acquisition request, for 500 ms
			// due to proc 1 reception delay. By this time, proc 2 should be
			// able to get its response from this proc
			std::string data = comm.acquire(0, 1);
			ASSERT_EQ(data, "0");
			comm.giveBack(0, 1);

			ASSERT_EQ(this->handler.data[2], 10);
		}
		else if (comm.getRank() == 2) {
			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = comm.acquire(2, 0);
			ASSERT_EQ(data, "2");

			this->handler.data[2] = std::stoul(data);
			this->handler.data[2] = 10;

			comm.giveBack(2, 0);
		}
		comm.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(acquire_from_acquiring_procs_test, 3);
	}

}
