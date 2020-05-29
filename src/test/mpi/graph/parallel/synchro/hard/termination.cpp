#include "graph/parallel/synchro/hard/termination.h"

#include <thread>
#include <chrono>

#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"
#include "communication/communication.h"

using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using FPMAS::graph::parallel::synchro::hard::TerminationAlgorithm;

class Mpi_TerminationTest : public ::testing::Test {
	protected:
		FPMAS::communication::MpiCommunicator comm;
		MockMutexServer<int> mutex_server;
		TerminationAlgorithm<FPMAS::communication::TypedMpi> termination {comm};

		void SetUp() override {
			EXPECT_CALL(mutex_server, getEpoch).WillRepeatedly(Return(Epoch::EVEN));
			EXPECT_CALL(mutex_server, setEpoch(Epoch::ODD));
		}

};

TEST_F(Mpi_TerminationTest, simple_termination_test) {
	EXPECT_CALL(mutex_server, handleIncomingRequests).Times(AnyNumber());
	termination.terminate(mutex_server);
}


TEST_F(Mpi_TerminationTest, termination_test_with_delay) {
	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		EXPECT_CALL(mutex_server, handleIncomingRequests).Times(AnyNumber());
		std::this_thread::sleep_for(1s);
	} else {
		EXPECT_CALL(mutex_server, handleIncomingRequests).Times(AtLeast(1));
	}

	termination.terminate(mutex_server);
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);
}
