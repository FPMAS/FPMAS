#include "gmock/gmock.h"

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

using testing::Return;
using testing::ReturnRef;
using testing::ReturnPointee;
using testing::InSequence;
using testing::SetArgReferee;
using testing::SaveArg;
using testing::_;
using testing::An;
using testing::AnyNumber;

#include "gtest/gtest.h"
#include "utils/test.h"

#include "api/communication/mock_communication.h"
#include "api/communication/mock_readers_writers.h"

#include "communication/request_handler.h"

using FPMAS::communication::RequestHandler;

class TerminationTest : public ::testing::Test {
	protected:
		MockResourceHandler handler;
		MockMpiCommunicator comm {0, 4};
		RequestHandler<MockMpiCommunicator> requestHandler {comm, handler};

};

TEST_F(TerminationTest, rank_0_white_tokens) {
	EXPECT_CALL(comm, send(Color::WHITE, 3))
		.Times(1);

	EXPECT_CALL(comm, Iprobe(1, Tag::TOKEN, _))
		.WillRepeatedly(Return(1));

	EXPECT_CALL(comm, recv(_, An<Color&>()))
		.WillOnce(DoAll(
				SetArgReferee<1>(Color::WHITE),
				Return()
				));

	// Sends end messages
	EXPECT_CALL(comm, sendEnd(1)).Times(1);
	EXPECT_CALL(comm, sendEnd(2)).Times(1);
	EXPECT_CALL(comm, sendEnd(3)).Times(1);

	requestHandler.terminate();
}

class Mpi_TerminationTest : public ::testing::Test {
	protected:
		MockResourceHandler handler;
		FPMAS::communication::MpiCommunicator comm;
		RequestHandler<FPMAS::communication::MpiCommunicator> requestHandler {comm, handler};

};

TEST_F(Mpi_TerminationTest, simple_termination_test) {
	requestHandler.terminate();
}


TEST_F(Mpi_TerminationTest, termination_test_with_delay) {
	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		std::this_thread::sleep_for(1s);
	}

	requestHandler.terminate();
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);
}

class LocalReadAcquireTest : public ::testing::Test {
	protected:
		const int RANK = 0;
		MockReadersWriters readersWriters;
		MockResourceHandler handler;
		MockMpiCommunicator comm {RANK, 16};
		RequestHandler<MockMpiCommunicator, MockReadersWritersManager> requestHandler {comm, handler};

		void SetUp() override {
			ON_CALL(comm, Iprobe)
				.WillByDefault(Return(false));
			EXPECT_CALL(comm, Iprobe)
				.Times(AnyNumber());

			EXPECT_CALL(
				const_cast<MockReadersWritersManager&>(
					requestHandler.getReadersWritersManager()
					),
				AccessOp(DistributedId(0, 0))
				).WillRepeatedly(ReturnRef(readersWriters));
		}

};

TEST_F(LocalReadAcquireTest, self_read) {
	EXPECT_CALL(readersWriters, isLocked).WillOnce(Return(false));
	EXPECT_CALL(handler, getLocalData(DistributedId(RANK, RANK))).WillOnce(Return(std::to_string(RANK)));
	std::string data = requestHandler.read(DistributedId(RANK, RANK), RANK);
	ASSERT_EQ(data, std::to_string(RANK));
}

TEST_F(LocalReadAcquireTest, self_acquire) {
	EXPECT_CALL(readersWriters, isLocked).WillOnce(Return(false));
	EXPECT_CALL(readersWriters, lock).Times(1);

	EXPECT_CALL(handler, getLocalData(DistributedId(RANK, RANK))).WillOnce(Return(std::to_string(RANK)));

	std::string data = requestHandler.acquire(DistributedId(RANK, RANK), RANK);
	
	// No need for those calls, this data should be modified in place
	EXPECT_CALL(handler, getUpdatedData)
		.Times(0);
	EXPECT_CALL(handler, updateData)
		.Times(0);
	
	EXPECT_CALL(readersWriters, release).Times(1);

	requestHandler.giveBack(DistributedId(RANK, RANK), RANK);
}

TEST_F(LocalReadAcquireTest, self_read_while_acquired) {
	EXPECT_CALL(readersWriters, isLocked)
		.Times(5)
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(false));


	EXPECT_CALL(handler, getLocalData(DistributedId(0, 0)))
		.WillOnce(Return("5"));
	auto data = requestHandler.read(DistributedId(0, 0), RANK);
	ASSERT_EQ(data, "5");
}

TEST_F(LocalReadAcquireTest, self_acquire_while_acquired) {
	EXPECT_CALL(readersWriters, isLocked)
		.Times(5)
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(true))
		.WillOnce(Return(false));

	EXPECT_CALL(handler, getLocalData(DistributedId(0, 0)))
		.WillOnce(Return("5"));

	EXPECT_CALL(readersWriters, lock).Times(1);
	auto data = requestHandler.acquire(DistributedId(0, 0), RANK);

	EXPECT_CALL(readersWriters, release).Times(1);
	requestHandler.giveBack(DistributedId(0, 0), RANK);

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

class Mpi_ReadAcquireTest : public ::testing::Test {
	protected:
		MockResourceHandler handler;
		FPMAS::communication::MpiCommunicator comm;
		RequestHandler<FPMAS::communication::MpiCommunicator> requestHandler {comm, handler};
};

/**
 * Tests if its possible to read a data from a passive proc (ie a proc waiting
 * for termination after terminate() has been called.
 */
TEST_F(Mpi_ReadAcquireTest, read_from_passive_procs_test) {
	if(comm.getSize() >= 2) {
		// Mock set up
		if(comm.getRank() % 2 == 1) {
			EXPECT_CALL(handler, getLocalData(
						DistributedId(comm.getRank() ,comm.getRank() - 1))
					).WillOnce(Return(std::to_string(comm.getRank() - 1)));
		} else {
			EXPECT_CALL(handler, getLocalData).Times(0);
		}

		// Each even proc asks for data to proc + 1 (if it exists)
		if(comm.getRank() % 2 == 0 && comm.getRank() != comm.getSize() - 1) {
			// Ensures that even procs enter passive mode
			std::this_thread::sleep_for(100ms);

			std::string data = requestHandler.read(DistributedId(comm.getRank()+1, comm.getRank()), comm.getRank() + 1);
			ASSERT_EQ(data, std::to_string(comm.getRank()));
		}
		// Odd procs immediatly enter passive mode
		requestHandler.terminate();
	} else {
		PRINT_MIN_PROCS_WARNING(read_from_passive_procs_test, 2);
	}
}

/**
 * Tests if its possible to read a data from an active proc performing an
 * acquisition (ie a proc waiting for a response after acquie() has been called)
 */
TEST_F(Mpi_ReadAcquireTest, read_from_acquiring_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Respond to request from 0
			EXPECT_CALL(handler, getLocalData(DistributedId(1, 0)))
				.WillOnce(Return("0"));
			// Receive updated data from 0
			EXPECT_CALL(handler, updateData(DistributedId(1, 0), "10")).Times(1);

			// Delays the acquire request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Read request from 2
			EXPECT_CALL(handler, getLocalData(DistributedId(0, 2)))
				.WillOnce(Return("2"));

			// Give back data with an updated value
			EXPECT_CALL(handler, getUpdatedData(DistributedId(1, 0))).WillOnce(Return("10"));

			// Will stay active for 500 ms due to proc 1 reception delay
			// By this time, proc 2 should be able to get its response from
			// this proc
			std::string data = requestHandler.acquire(DistributedId(1, 0), 1);
			ASSERT_EQ(data, "0");

			requestHandler.giveBack(DistributedId(1, 0), 1);
		}
		else if (comm.getRank() == 2) {
			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);

			// Asks data to the currently active proc
			std::string data = requestHandler.read(DistributedId(0, 2), 0);
			ASSERT_EQ(data, "2");
		}
		requestHandler.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(read_from_acquiring_procs_test, 3);
	}
}

/**
 * Tests if its possible to read a data from an active proc performing a
 * reading (ie a proc waiting for a response after read() has been called)
 */
TEST_F(Mpi_ReadAcquireTest, read_from_reading_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			EXPECT_CALL(handler, getLocalData(DistributedId(1, 0))).WillOnce(Return("0"));

			// Delays the read request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			EXPECT_CALL(handler, getLocalData(DistributedId(0, 2))).WillOnce(Return("2"));

			// Will stay active for 500 ms due to proc 1 reception delay
			// By this time, proc 2 should be able to get its response from
			// this proc
			std::string data = requestHandler.read(DistributedId(1, 0), 1);
			ASSERT_EQ(data, "0");
		}
		else if (comm.getRank() == 2) {
			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = requestHandler.read(DistributedId(0, 2), 0);
			ASSERT_EQ(data, "2");
		}
		requestHandler.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(read_from_reading_procs_test, 3);
	}

}

/**
 * Tests if its possible to acquire a data from a passive proc (ie a proc waiting
 * for termination after terminate() has been called.
 */
TEST_F(Mpi_ReadAcquireTest, acquire_from_passive_procs_test) {
	if(comm.getSize() >= 2) {
		// Mock set up
		if(comm.getRank() % 2 == 1) {
			EXPECT_CALL(handler, getLocalData(
						DistributedId(comm.getRank() ,comm.getRank() - 1))
					).WillOnce(Return(std::to_string(comm.getRank() - 1)));
			EXPECT_CALL(handler, updateData(
						DistributedId(comm.getRank(), comm.getRank()-1),
						std::to_string(comm.getRank())
						)).Times(1);
		} else {
			EXPECT_CALL(handler, getLocalData).Times(0);
			// Each even proc asks for data to proc + 1 (if it exists)
			if(comm.getRank() != comm.getSize() - 1) {
				// Ensures that even procs enter passive mode
				std::this_thread::sleep_for(100ms);

				std::string data_str = requestHandler.acquire(DistributedId(comm.getRank() + 1, comm.getRank()), comm.getRank() + 1);
				ASSERT_EQ(data_str, std::to_string(comm.getRank()));

				EXPECT_CALL(handler, getUpdatedData(DistributedId(comm.getRank()+1, comm.getRank())))
					.WillOnce(Return(std::to_string(comm.getRank()+1)));

				requestHandler.giveBack(DistributedId(comm.getRank()+1, comm.getRank()), comm.getRank() + 1);
			}
		}
		// Odd procs immediatly enter passive mode
		requestHandler.terminate();

	} else {
		PRINT_MIN_PROCS_WARNING(acquire_from_passive_procs_test, 2);
	}
}

/**
 * Tests if it's possible to acquire a data from an active proc performing a
 * reading (ie a proc waiting for a response after read() has been called)
 */
TEST_F(Mpi_ReadAcquireTest, acquire_from_reading_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Respond to request from 0
			EXPECT_CALL(handler, getLocalData(DistributedId(1, 0)))
				.WillOnce(Return("0"));

			// Delays the read request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			// Handles acquire from 2
			EXPECT_CALL(handler, getLocalData(DistributedId(0, 2)))
				.WillOnce(Return("2"));
			EXPECT_CALL(handler, updateData(DistributedId(0, 2), "10"))
				.Times(1);

			// Will stay active, waiting for the reading request, for 500 ms
			// due to proc 1 reception delay By this time, proc 2 should be
			// able to get its response from this proc
			std::string data = requestHandler.read(DistributedId(1, 0), 1);
			ASSERT_EQ(data, "0");
		}
		else if (comm.getRank() == 2) {
			// Give back updated data to 0
			EXPECT_CALL(handler, getUpdatedData(DistributedId(0, 2)))
				.WillOnce(Return("10"));

			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = requestHandler.acquire(DistributedId(0, 2), 0);
			ASSERT_EQ(data, "2");

			requestHandler.giveBack(DistributedId(0, 2), 0);
		}
		requestHandler.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(acquire_from_reading_procs_test, 3);
	}

}

/**
 * Tests if it's possible to acquire a data from an active proc performing an
 * acquisition (ie a proc waiting for a response after acquire() has been called)
 */
TEST_F(Mpi_ReadAcquireTest, acquire_from_acquiring_procs_test) {
	if(comm.getSize() >= 3) {
		if(comm.getRank() == 1) {
			// Acquire request from 0
			EXPECT_CALL(handler, getLocalData(DistributedId(1, 0)))
				.WillOnce(Return("0"));
			EXPECT_CALL(handler, updateData(DistributedId(1, 0), "10"))
				.Times(1);

			// Delays the acquire request reception from 0
			std::this_thread::sleep_for(500ms);
		}
		else if (comm.getRank() == 0) {
			{
				InSequence s;
				// While waiting for acquire request to complete, respond to
				// acquire from 2
				EXPECT_CALL(handler, getLocalData(DistributedId(0, 2)))
					.WillOnce(Return("2"));
				EXPECT_CALL(handler, updateData(DistributedId(0, 2), "20"))
					.Times(1);

				// Finally complete acquire request
				EXPECT_CALL(handler, getUpdatedData(DistributedId(1, 0)))
					.WillOnce(Return("10"));
			}
			// Will stay active, waiting for the acquisition request, for 500 ms
			// due to proc 1 reception delay. By this time, proc 2 should be
			// able to get its response from this proc
			std::string data = requestHandler.acquire(DistributedId(1, 0), 1);
			ASSERT_EQ(data, "0");
			requestHandler.giveBack(DistributedId(1, 0), 1);
		}
		else if (comm.getRank() == 2) {
			// Give back updated data to 0
			EXPECT_CALL(handler, getUpdatedData(DistributedId(0, 2)))
				.WillOnce(Return("20"));

			// Ensures that proc 0 has initiated its request
			std::this_thread::sleep_for(10ms);
			// Asks data to the currently active proc
			std::string data = requestHandler.acquire(DistributedId(0, 2), 0);
			ASSERT_EQ(data, "2");

			requestHandler.giveBack(DistributedId(0, 2), 0);
		}
		requestHandler.terminate();
	}
	else {
		PRINT_MIN_PROCS_WARNING(acquire_from_acquiring_procs_test, 3);
	}

}

TEST_F(Mpi_ReadAcquireTest, heavy_race_condition_test) {
	std::string proc_0_data_str = "0";
	if(comm.getRank() != 0) {
		std::string local_data;
		EXPECT_CALL(handler, getUpdatedData(DistributedId(0, 1)))
			.Times(500)
			.WillRepeatedly(ReturnPointee(&local_data));
		for (int i = 0; i < 500; i++) {
			local_data = requestHandler.acquire(DistributedId(0, 1), 0);
			local_data = std::to_string(std::stol(local_data) + 1);
			//handler.updateData(DistributedId(0, 1), std::to_string(std::stoi(data) + 1));
			requestHandler.giveBack(DistributedId(0, 1), 0);
		}
	} else {
		EXPECT_CALL(handler, updateData(DistributedId(0, 1), _))
			.Times(500 * (comm.getSize() - 1))
			.WillRepeatedly(SaveArg<1>(&proc_0_data_str));
		EXPECT_CALL(handler, getLocalData(DistributedId(0, 1)))
			.Times(500 * (comm.getSize() - 1))
			.WillRepeatedly(ReturnPointee(&proc_0_data_str));

	}
	requestHandler.terminate();
	if(comm.getRank() == 0) {
		ASSERT_EQ(std::stol(proc_0_data_str), 500 * (comm.getSize() - 1));
	}
}

TEST_F(Mpi_ReadAcquireTest, heavy_race_condition_test_including_local) {
	std::string local_data = "0";
	if(comm.getRank() != 0) {
		// Give back locally updated data to proc 0
		// Even if proc 0 acquires its own data, this is not called on proc 0
		// since updates are assumed to be local
		EXPECT_CALL(handler, getUpdatedData(DistributedId(0, 1)))
			.Times(500)
			.WillRepeatedly(ReturnPointee(&local_data));
	} else {
		// Proc 0 does not call this since updates are local
		EXPECT_CALL(handler, getUpdatedData(DistributedId(0, 1)))
			.Times(0);

		// Data acquisition
		EXPECT_CALL(handler, getLocalData(DistributedId(0, 1)))
			.Times(500 * comm.getSize())
			.WillRepeatedly(ReturnPointee(&local_data));

		// Received data updates
		EXPECT_CALL(handler, updateData(DistributedId(0, 1), _))
			.Times(500 * (comm.getSize() - 1))
			.WillRepeatedly(SaveArg<1>(&local_data));
	}
	for (int i = 0; i < 500; i++) {
		local_data = requestHandler.acquire(DistributedId(0, 1), 0);
		local_data = std::to_string(std::stol(local_data) + 1);
		requestHandler.giveBack(DistributedId(0, 1), 0);
	}
	requestHandler.terminate();
	if(comm.getRank() == 0) {
		ASSERT_EQ(std::stol(local_data), 500 * comm.getSize());
	}
}
