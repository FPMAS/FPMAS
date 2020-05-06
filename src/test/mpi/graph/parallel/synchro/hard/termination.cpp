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
		MockMutexServer<int> mutexServer;
		TerminationAlgorithm<FPMAS::communication::TypedMpi> termination {comm};

		void SetUp() override {
			EXPECT_CALL(mutexServer, getEpoch).WillRepeatedly(Return(Epoch::EVEN));
			EXPECT_CALL(mutexServer, setEpoch(Epoch::ODD));
		}

};

TEST_F(Mpi_TerminationTest, simple_termination_test) {
	EXPECT_CALL(mutexServer, handleIncomingRequests).Times(AnyNumber());
	termination.terminate(mutexServer);
}


TEST_F(Mpi_TerminationTest, termination_test_with_delay) {
	auto start = std::chrono::system_clock::now();
	if(comm.getRank() == 0) {
		EXPECT_CALL(mutexServer, handleIncomingRequests).Times(AnyNumber());
		std::this_thread::sleep_for(1s);
	} else {
		EXPECT_CALL(mutexServer, handleIncomingRequests).Times(AtLeast(1));
	}

	termination.terminate(mutexServer);
	auto end = std::chrono::system_clock::now();

	std::chrono::duration delay = end - start;

	ASSERT_GE(delay, 1s);
}
