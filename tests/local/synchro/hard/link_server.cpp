#include "fpmas/synchro/hard/hard_sync_linker.h"
#include "fpmas/synchro/hard/server_pack.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"
#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/synchro/hard/mock_hard_sync_mode.h"

using namespace testing;

using fpmas::graph::EdgePtrWrapper;
using fpmas::synchro::hard::api::Epoch;
using fpmas::synchro::hard::api::Tag;
using fpmas::synchro::hard::ServerPack;
using fpmas::synchro::hard::LinkServer;

class LinkServerTest : public Test {
	private:
		class MockProbe {
			private:
				int source;
			public:
				MockProbe(int source) : source(source) {}
				void operator()(int, int tag, fpmas::api::communication::Status& status) {
					status.source = this->source;
					status.tag = tag;
				}
		};

	protected:
		MockMpiCommunicator<7, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<EdgePtrWrapper<int>> edge_mpi {comm};

		MockDistributedGraph<
			int,
			MockDistributedNode<int, NiceMock>,
			MockDistributedEdge<int, NiceMock>
			> mock_graph;
		decltype(mock_graph)::EdgeMap edge_map;

		MockHardSyncLinker<int> mock_sync_linker;
		LinkServer<int> link_server;

		MockTerminationAlgorithm mock_termination;
		MockMutexServer<int> mock_mutex_server;
		ServerPack<int> server_pack;

		LinkServerTest() :
			link_server(comm, mock_graph, mock_sync_linker, id_mpi, edge_mpi),
			server_pack(comm, mock_termination, mock_mutex_server, link_server) {}

		void SetUp() override {
			ON_CALL(id_mpi, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(id_mpi, Iprobe).Times(AnyNumber());
			ON_CALL(edge_mpi, Iprobe).WillByDefault(Return(false));
			EXPECT_CALL(edge_mpi, Iprobe).Times(AnyNumber());
			ON_CALL(mock_graph, getEdges).WillByDefault(ReturnRef(edge_map));
			EXPECT_CALL(mock_graph, getEdges).Times(AnyNumber());
		}

		void expectLink(int source, MockDistributedEdge<int, NiceMock>* mock_edge) {
			Expectation probe = EXPECT_CALL(edge_mpi, Iprobe(_, Epoch::EVEN | Tag::LINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(edge_mpi, recv(source, Epoch::EVEN | Tag::LINK, _))
				.After(probe)
				.WillOnce(Return(mock_edge));

			EXPECT_CALL(mock_graph, importEdge(mock_edge));
		}

		void expectUnlink(int source, MockDistributedEdge<int, NiceMock>* mock_edge) {
			edge_map.insert({mock_edge->getId(), mock_edge});
			EXPECT_CALL(mock_graph, getEdge(mock_edge->getId()))
				.WillRepeatedly(Return(mock_edge));
			Expectation probe = EXPECT_CALL(id_mpi, Iprobe(_, Epoch::EVEN | Tag::UNLINK, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(source, Epoch::EVEN | Tag::UNLINK, _))
				.After(probe)
				.WillOnce(Return(mock_edge->getId()));
			EXPECT_CALL(mock_graph, erase(mock_edge));
		}

		void expectRemoveNode(int source, fpmas::api::graph::DistributedNode<int>* node) {
			EXPECT_CALL(mock_graph, getNode(node->getId()))
				.WillRepeatedly(Return(node));
			Expectation probe = EXPECT_CALL(id_mpi, Iprobe(_, Epoch::EVEN | Tag::REMOVE_NODE, _))
				.WillOnce(DoAll(Invoke(MockProbe(source)), Return(true)));

			EXPECT_CALL(id_mpi, recv(source, Epoch::EVEN | Tag::REMOVE_NODE, _))
				.After(probe)
				.WillOnce(Return(node->getId()));

			EXPECT_CALL(mock_graph, removeNode(node));
		}

};

TEST_F(LinkServerTest, epoch) {
	link_server.setEpoch(Epoch::EVEN);
	ASSERT_EQ(link_server.getEpoch(), Epoch::EVEN);
	link_server.setEpoch(Epoch::ODD);
	ASSERT_EQ(link_server.getEpoch(), Epoch::ODD);
}

TEST_F(LinkServerTest, handleLink) {
	MockDistributedEdge<int, NiceMock> mock_edge {DistributedId(15, 2), 7};
	expectLink(4, &mock_edge);

	link_server.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleUnlink) {
	MockDistributedEdge<int, NiceMock> mock_edge {{3, 5}, 7};
	MockDistributedNode<int, NiceMock> mock_node;
	ON_CALL(mock_edge, getSourceNode)
		.WillByDefault(Return(&mock_node));
	ON_CALL(mock_edge, getTargetNode)
		.WillByDefault(Return(&mock_node));

	expectUnlink(6, &mock_edge);

	link_server.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleRemoveNode) {
	MockDistributedNode<int, NiceMock> mock_node {{0, 2}};
	MockDistributedEdge<int, NiceMock> out_edge;
	MockDistributedEdge<int, NiceMock> in_edge_1;
	MockDistributedEdge<int, NiceMock> in_edge_2;
	ON_CALL(mock_node, getIncomingEdges())
		.WillByDefault(Return(
					std::vector<fpmas::api::graph::DistributedEdge<int>*>
					{&in_edge_1, &in_edge_2}));
	ON_CALL(mock_node, getOutgoingEdges())
		.WillByDefault(Return(
					std::vector<fpmas::api::graph::DistributedEdge<int>*>
					{&out_edge}));
	ON_CALL(out_edge, getSourceNode())
		.WillByDefault(Return(&mock_node));
	ON_CALL(in_edge_1, getTargetNode())
		.WillByDefault(Return(&mock_node));
	ON_CALL(in_edge_2, getTargetNode())
		.WillByDefault(Return(&mock_node));
	// Fake set up
	MockDistributedNode<int, NiceMock> fake_node;
	ON_CALL(out_edge, getTargetNode())
		.WillByDefault(Return(&fake_node));
	ON_CALL(in_edge_1, getSourceNode())
		.WillByDefault(Return(&fake_node));
	ON_CALL(in_edge_2, getSourceNode())
		.WillByDefault(Return(&fake_node));

	expectRemoveNode(4, &mock_node);

	link_server.handleIncomingRequests();
}

TEST_F(LinkServerTest, handleAll) {
	MockDistributedEdge<int, NiceMock> link_mock_edge {{15, 2}, 7};
	MockDistributedEdge<int, NiceMock> unlink_mock_edge {{3, 5}, 7};
	MockDistributedNode<int, NiceMock> mock_node {{2, 7}};

	MockDistributedNode<int, NiceMock> fake_node;
	ON_CALL(unlink_mock_edge, getSourceNode)
		.WillByDefault(Return(&fake_node));
	ON_CALL(unlink_mock_edge, getTargetNode)
		.WillByDefault(Return(&fake_node));

	expectLink(4, &link_mock_edge);
	expectUnlink(6, &unlink_mock_edge);
	expectRemoveNode(5, &mock_node);

	link_server.handleIncomingRequests();
}
