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

using fpmas::graph::parallel::EdgePtrWrapper;
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
		MockMpi<EdgePtrWrapper<int>> edge_mpi {comm};

		MockDistributedGraph<
			int,
			MockDistributedNode<int>,
			MockDistributedEdge<int>
			> mock_graph;

		LinkServer<int> linkServer
			{comm, mock_graph, id_mpi, edge_mpi};

		void SetUp() override {
			ON_CALL(comm, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(comm, Iprobe).Times(AnyNumber());
		}

		void expectLink(int source, MockDistributedEdge<int>* mock_edge) {
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::LINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(edge_mpi, recv(source, Epoch::EVEN | Tag::LINK, _))
				.After(probe)
				.WillOnce(Return(mock_edge));

			EXPECT_CALL(mock_graph, importEdge(mock_edge));
		}

		void expectUnlink(int source, MockDistributedEdge<int>* mock_edge) {
			EXPECT_CALL(mock_graph, getEdge(mock_edge->getId()))
				.WillRepeatedly(Return(mock_edge));
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::UNLINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(source, Epoch::EVEN | Tag::UNLINK, _))
				.After(probe)
				.WillOnce(Return(mock_edge->getId()));
			EXPECT_CALL(mock_graph, erase(mock_edge));
		}
};

TEST_F(LinkServerTest, epoch) {
	linkServer.setEpoch(Epoch::EVEN);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::EVEN);
	linkServer.setEpoch(Epoch::ODD);
	ASSERT_EQ(linkServer.getEpoch(), Epoch::ODD);
}

TEST_F(LinkServerTest, handleLink) {
	MockDistributedEdge<int> mock_edge {DistributedId(15, 2), 7};
	expectLink(4, &mock_edge);

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleUnlink) {
	MockDistributedEdge<int> mock_edge {{3, 5}, 7};
	expectUnlink(6, &mock_edge);

	linkServer.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleAll) {
	MockDistributedEdge<int> link_mock_edge {{15, 2}, 7};
	MockDistributedEdge<int> unlink_mock_edge {{3, 5}, 7};
	expectLink(4, &link_mock_edge);
	expectUnlink(6, &unlink_mock_edge);

	linkServer.handleIncomingRequests();
}
