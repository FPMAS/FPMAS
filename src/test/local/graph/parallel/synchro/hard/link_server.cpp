#include "graph/parallel/synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"

using ::testing::DoAll;
using ::testing::Expectation;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Return;
using ::testing::ReturnPointee;

using FPMAS::graph::parallel::ArcPtrWrapper;
using FPMAS::graph::parallel::synchro::hard::Epoch;
using FPMAS::graph::parallel::synchro::hard::Tag;
using FPMAS::graph::parallel::synchro::hard::LinkServer;

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
			{comm, id_mpi, arc_mpi, mock_graph};

		void SetUp() override {
			ON_CALL(comm, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(comm, Iprobe).Times(AnyNumber());
		}

		void expectLink(int source) {
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::LINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			MockDistributedArc<int> mock_arc {DistributedId(15, 2), 7};
			EXPECT_CALL(arc_mpi, recv(_))
				.After(probe)
				.WillOnce(Return(&mock_arc));

			EXPECT_CALL(mock_graph, importArc(Property(
					&FPMAS::api::graph::parallel::DistributedArc<int>::getId, mock_arc.getId())));
		}

		void expectUnlink(int source, DistributedId id) {
			MockDistributedArc<int> mock_arc {id, 7};
			EXPECT_CALL(mock_graph, getArc(id))
				.WillRepeatedly(Return(&mock_arc));
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::UNLINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(_))
				.After(probe)
				.WillOnce(Return(id));
			EXPECT_CALL(mock_graph, clear(&mock_arc));
		}
};

TEST_F(LinkServerTest, epoch) {
	linkServer.setEpoch(Epoch::EVEN);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::EVEN);
	linkServer.setEpoch(Epoch::ODD);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::ODD);
}

TEST_F(LinkServerTest, handleLink) {
	expectLink(4);

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleUnlink) {
	expectUnlink(6, {3, 5});

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleAll) {
	expectLink(4);
	expectUnlink(6, {3, 5});

	linkServer.handleIncomingRequests();
}
