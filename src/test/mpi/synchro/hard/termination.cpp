#include "synchro/hard/termination.h"

#include <thread>
#include <chrono>

#include "../mocks/synchro/hard/mock_client_server.h"
#include "communication/communication.h"

using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using FPMAS::synchro::hard::TerminationAlgorithm;

class TerminationTest : public ::testing::Test {
	protected:
		FPMAS::communication::MpiCommunicator comm;
		MockMutexServer<int> mutex_server;
		TerminationAlgorithm<FPMAS::communication::TypedMpi> termination {comm};

		void SetUp() override {
			EXPECT_CALL(mutex_server, getEpoch).WillRepeatedly(Return(Epoch::EVEN));
			EXPECT_CALL(mutex_server, setEpoch(Epoch::ODD));
		}

};

TEST_F(TerminationTest, simple_termination_test) {
	EXPECT_CALL(mutex_server, handleIncomingRequests).Times(AnyNumber());
	termination.terminate(mutex_server);
}


TEST_F(TerminationTest, termination_test_with_delay) {
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

class FakeServer : public FPMAS::api::synchro::hard::Server {
	private:
		Epoch epoch = Epoch::EVEN;
	public:
		void setEpoch(Epoch epoch) override {this->epoch = epoch;};
		Epoch getEpoch() const override {return epoch;}

		void handleIncomingRequests() override {}
};

class MultipleTerminationTest : public ::testing::Test {
	protected:
		FPMAS::communication::MpiCommunicator comm;
		FakeServer server1;
		FakeServer server2;
		TerminationAlgorithm<FPMAS::communication::TypedMpi> termination {comm};
};

TEST_F(MultipleTerminationTest, test) {
	for(int i = 0; i < 50; i++) {
		termination.terminate(server1);
		MPI_Barrier(comm.getMpiComm());

		termination.terminate(server2);
		MPI_Barrier(comm.getMpiComm());
	}
}
