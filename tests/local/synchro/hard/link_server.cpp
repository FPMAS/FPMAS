#include "fpmas/synchro/hard/hard_sync_linker.h"
#include "fpmas/synchro/hard/server_pack.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"
#include "../mocks/synchro/mock_sync_mode.h"

using ::testing::DoAll;
using ::testing::Expectation;
using ::testing::Invoke;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Return;
using ::testing::ReturnPointee;

using fpmas::graph::EdgePtrWrapper;
using fpmas::synchro::hard::api::Epoch;
using fpmas::synchro::hard::api::Tag;
using fpmas::synchro::hard::ServerPack;
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
		decltype(mock_graph)::EdgeMap edge_map;

		fpmas::synchro::hard::HardDataSync<int> data_sync;
		LinkServer<int> link_server;

		MockTerminationAlgorithm mock_termination;
		MockMutexServer<int> mock_mutex_server;
		ServerPack<int> server_pack;

		LinkServerTest() :
			data_sync(comm, server_pack, mock_graph),
			link_server(comm, mock_graph, data_sync, id_mpi, edge_mpi),
			server_pack(comm, mock_termination, mock_mutex_server, link_server) {}

		void SetUp() override {
			ON_CALL(comm, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(comm, Iprobe).Times(AnyNumber());
			ON_CALL(mock_graph, getEdges).WillByDefault(ReturnRef(edge_map));
			EXPECT_CALL(mock_graph, getEdges).Times(AnyNumber());
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
			edge_map.insert({mock_edge->getId(), mock_edge});
			EXPECT_CALL(mock_graph, getEdge(mock_edge->getId()))
				.WillRepeatedly(Return(mock_edge));
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::UNLINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(source, Epoch::EVEN | Tag::UNLINK, _))
				.After(probe)
				.WillOnce(Return(mock_edge->getId()));
			EXPECT_CALL(mock_graph, erase(mock_edge));
		}

		void expectRemoveNode(int source, MockDistributedNode<int>* mock_node) {
			EXPECT_CALL(mock_graph, getNode(mock_node->getId()))
				.WillRepeatedly(Return(mock_node));
			Expectation probe = EXPECT_CALL(comm, Iprobe(_, Epoch::EVEN | Tag::REMOVE_NODE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(source, Epoch::EVEN | Tag::REMOVE_NODE, _))
				.After(probe)
				.WillOnce(Return(mock_node->getId()));
			for(auto edge : mock_node->getIncomingEdges())
				EXPECT_CALL(mock_graph, unlink(edge));
			for(auto edge : mock_node->getOutgoingEdges())
				EXPECT_CALL(mock_graph, unlink(edge));
			EXPECT_CALL(mock_graph, erase(mock_node)).Times(0);
		}

};

TEST_F(LinkServerTest, epoch) {
	link_server.setEpoch(Epoch::EVEN);
	ASSERT_EQ(link_server.getEpoch(), Epoch::EVEN);
	link_server.setEpoch(Epoch::ODD);
	ASSERT_EQ(link_server.getEpoch(), Epoch::ODD);
}

TEST_F(LinkServerTest, handleLink) {
	MockDistributedEdge<int> mock_edge {DistributedId(15, 2), 7};
	expectLink(4, &mock_edge);

	link_server.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleUnlink) {
	MockDistributedEdge<int> mock_edge {{3, 5}, 7};
	expectUnlink(6, &mock_edge);

	link_server.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleRemoveNode) {
	MockDistributedNode<int> mock_node {{0, 2}};
	MockDistributedEdge<int> out_edge;
	MockDistributedEdge<int> in_edge_1;
	MockDistributedEdge<int> in_edge_2;
	EXPECT_CALL(mock_node, getIncomingEdges())
		.Times(AnyNumber())
		.WillRepeatedly(Return(
					std::vector<fpmas::api::graph::DistributedEdge<int>*>
					{&in_edge_1, &in_edge_2}));
	EXPECT_CALL(mock_node, getOutgoingEdges())
		.Times(AnyNumber())
		.WillRepeatedly(Return(
					std::vector<fpmas::api::graph::DistributedEdge<int>*>
					{&out_edge}));
	expectRemoveNode(4, &mock_node);

	link_server.handleIncomingRequests();

	Expectation terminate = EXPECT_CALL(mock_termination, terminate);
	EXPECT_CALL(mock_graph, erase(&mock_node))
		.After(terminate);
	data_sync.synchronize();
}

TEST_F(LinkServerTest, handleAll) {
	MockDistributedEdge<int> link_mock_edge {{15, 2}, 7};
	MockDistributedEdge<int> unlink_mock_edge {{3, 5}, 7};
	MockDistributedNode<int> mock_node {{2, 7}};
	expectLink(4, &link_mock_edge);
	expectUnlink(6, &unlink_mock_edge);
	expectRemoveNode(5, &mock_node);

	link_server.handleIncomingRequests();

	Expectation terminate = EXPECT_CALL(mock_termination, terminate);
	EXPECT_CALL(mock_graph, erase(&mock_node))
		.After(terminate);
	data_sync.synchronize();
}
