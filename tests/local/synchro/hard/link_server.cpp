#include "fpmas/synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/synchro/mock_mutex.h"

using ::testing::DoAll;
using ::testing::Expectation;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Return;
using ::testing::ReturnPointee;

using fpmas::graph::parallel::ArcPtrWrapper;
using fpmas::synchro::hard::Epoch;
using fpmas::synchro::hard::Tag;
using fpmas::synchro::hard::LinkServer;

class LinkServerTest : public ::testing::Test {
	private:
		class MockProbe {
			private:
				int source;
			public:
				MockProbe(int source) : source(source) {}
				void operator()(int, int tag, MPI_Status* status) {
					status->MPI_SOURCE = this->source;
					status->MPI_TAG = tag;
				}
		};

	protected:
		MockMpiCommunicator<7, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<ArcPtrWrapper<int>> arc_mpi {comm};

		MockDistributedGraph<
			int,
			MockDistributedNode<int>,
			MockDistributedArc<int>
			> mock_graph;

		LinkServer<int> linkServer
			{comm, mock_graph, id_mpi, arc_mpi};

		void SetUp() override {
			ON_CALL(comm, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(comm, Iprobe).Times(AnyNumber());
		}

		void expectLink(int source, MockDistributedArc<int>* mock_arc) {
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::LINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(arc_mpi, recv(_))
				.After(probe)
				.WillOnce(Return(mock_arc));

			EXPECT_CALL(mock_graph, importArc(mock_arc));
		}

		void expectUnlink(int source, MockDistributedArc<int>* mock_arc) {
			EXPECT_CALL(mock_graph, getArc(mock_arc->getId()))
				.WillRepeatedly(Return(mock_arc));
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::UNLINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(_))
				.After(probe)
				.WillOnce(Return(mock_arc->getId()));
			EXPECT_CALL(mock_graph, erase(mock_arc));
		}
};

TEST_F(LinkServerTest, epoch) {
	linkServer.setEpoch(Epoch::EVEN);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::EVEN);
	linkServer.setEpoch(Epoch::ODD);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::ODD);
}

TEST_F(LinkServerTest, handleLink) {
	MockDistributedArc<int> mock_arc {DistributedId(15, 2), 7};
	expectLink(4, &mock_arc);

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleUnlink) {
	MockDistributedArc<int> mock_arc {{3, 5}, 7};
	expectUnlink(6, &mock_arc);

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleAll) {
	MockDistributedArc<int> link_mock_arc {{15, 2}, 7};
	MockDistributedArc<int> unlink_mock_arc {{3, 5}, 7};
	expectLink(4, &link_mock_arc);
	expectUnlink(6, &unlink_mock_arc);

	linkServer.handleIncomingRequests();
}
