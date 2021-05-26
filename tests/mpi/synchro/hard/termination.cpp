#include "fpmas/synchro/hard/termination.h"

#include <thread>
#include <chrono>

#include "synchro/hard/mock_client_server.h"
#include "fpmas/communication/communication.h"

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using fpmas::synchro::hard::TerminationAlgorithm;

class TerminationTest : public ::testing::Test {
	protected:
		fpmas::communication::MpiCommunicator comm;
		MockMutexServer<int> mutex_server;
		fpmas::communication::TypedMpi<fpmas::synchro::hard::api::Color> color_mpi {comm};
		TerminationAlgorithm termination {comm, color_mpi};

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
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} else {
		EXPECT_CALL(mutex_server, handleIncomingRequests).Times(AtLeast(1));
	}

	termination.terminate(mutex_server);
	auto end = std::chrono::system_clock::now();

	auto delay = end - start;

	ASSERT_GE(delay, std::chrono::seconds(1));
}

class FakeServer : public fpmas::synchro::hard::api::Server {
	private:
		Epoch epoch = Epoch::EVEN;
	public:
		void setEpoch(Epoch epoch) override {this->epoch = epoch;};
		Epoch getEpoch() const override {return epoch;}

		void handleIncomingRequests() override {}
};

class MultipleTerminationTest : public ::testing::Test {
	protected:
		fpmas::communication::MpiCommunicator comm;
		FakeServer server1;
		FakeServer server2;
		fpmas::communication::TypedMpi<fpmas::synchro::hard::api::Color> color_mpi {comm};
		TerminationAlgorithm termination {comm, color_mpi};
};

TEST_F(MultipleTerminationTest, test) {
	for(int i = 0; i < 50; i++) {
		termination.terminate(server1);
		MPI_Barrier(comm.getMpiComm());

		termination.terminate(server2);
		MPI_Barrier(comm.getMpiComm());
	}
}
