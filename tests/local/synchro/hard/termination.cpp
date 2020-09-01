#include "fpmas/synchro/hard/termination.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;

using fpmas::synchro::hard::Color;
using fpmas::synchro::hard::Tag;
using fpmas::synchro::hard::TerminationAlgorithm;

class TerminationTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<0, 4> comm;
		MockMutexServer<int> mutexServer;
		MockMpi<Color> mock_mpi {comm};
		TerminationAlgorithm termination {comm, mock_mpi};
};

class SetMpiStatus {
	public:
		void operator()(int source, int tag, fpmas::communication::Status& status) {
			status.source = source;
			status.tag = tag;
		}
};

TEST_F(TerminationTest, rank_0_white_tokens) {
	EXPECT_CALL(mutexServer, getEpoch).WillRepeatedly(Return(Epoch::EVEN));
	EXPECT_CALL(mutexServer, setEpoch(Epoch::ODD));

	EXPECT_CALL(mock_mpi, send(Color::WHITE, 3, Tag::TOKEN));

	EXPECT_CALL(mock_mpi, Iprobe(1, Tag::TOKEN, _))
		.WillRepeatedly(DoAll(Invoke(SetMpiStatus()), Return(1)));

	EXPECT_CALL(mock_mpi, recv(1, Tag::TOKEN, _))
		.WillOnce(Return(Color::WHITE));

	// Sends end messages
	EXPECT_CALL(comm, send(1, Tag::END)).Times(1);
	EXPECT_CALL(comm, send(2, Tag::END)).Times(1);
	EXPECT_CALL(comm, send(3, Tag::END)).Times(1);

	termination.terminate(mutexServer);
}
