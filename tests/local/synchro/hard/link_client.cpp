#include "fpmas/synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_edge.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using namespace testing;

using fpmas::graph::EdgePtrWrapper;
using fpmas::synchro::hard::api::Tag;
using fpmas::synchro::hard::LinkClient;

class LinkClientTest : public Test {
	protected:
		static const int THIS_RANK = 7;
		const DistributedId edge_id {9, 2};
		MockMpiCommunicator<THIS_RANK, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<EdgePtrWrapper<int>> edge_mpi {comm};

		MockTerminationAlgorithm mock_termination;
		MockMutexServer<int> mock_mutex_server;
		MockLinkServer mock_link_server;
		fpmas::synchro::hard::ServerPack<int> server_pack {
			comm, mock_termination, mock_mutex_server, mock_link_server};
		LinkClient<int> link_client {comm, id_mpi, edge_mpi, server_pack};

		DistributedId src_id {3, 6};
		MockDistributedNode<int, NiceMock> mock_src {src_id};
		MockDistributedNode<int, NiceMock> mock_tgt;

		MockDistributedEdge<int, NiceMock> mock_edge;

		void SetUp() override {
			ON_CALL(mock_edge, getId)
				.WillByDefault(Return(edge_id));

			ON_CALL(mock_edge, getSourceNode)
				.WillByDefault(Return(&mock_src));

			ON_CALL(mock_edge, getTargetNode)
				.WillByDefault(Return(&mock_tgt));

			ON_CALL(mock_link_server, getEpoch).WillByDefault(Return(Epoch::EVEN));
			EXPECT_CALL(mock_link_server, getEpoch).Times(AnyNumber());
		}
};

TEST_F(LinkClientTest, link_local_src_distant_tgt) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(THIS_RANK));

	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(10));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_edge);
}

TEST_F(LinkClientTest, link_local_tgt_distant_src) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));

	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(THIS_RANK));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::LOCAL));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_edge);
}

TEST_F(LinkClientTest, link_distant_tgt_distant_src) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));
	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(12));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(
				Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(edge_mpi, Issend(
				Property(&EdgePtrWrapper<int>::get, &mock_edge), 12, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	link_client.link(&mock_edge);
}

// If src and tgt are on the same proc, the link request must be sent ONLY ONCE
TEST_F(LinkClientTest, link_distant_tgt_distant_src_same_location) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));
	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(10));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(
				Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true));

	link_client.link(&mock_edge);
}

// Should not do anything
TEST_F(LinkClientTest, unlink_local_src_local_tgt) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::LOCAL));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::LOCAL));

	EXPECT_CALL(id_mpi, Issend).Times(0);

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_local_src_distant_tgt) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(THIS_RANK));

	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(10));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));
	{
		InSequence s;
		EXPECT_CALL(id_mpi, Issend(edge_id, 10, Epoch::EVEN | Tag::UNLINK, _));
		EXPECT_CALL(comm, test).WillOnce(Return(true));
	}

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_local_tgt_distant_src) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::LOCAL));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));
	{
		InSequence s;
		EXPECT_CALL(id_mpi, Issend(edge_id, 10, Epoch::EVEN | Tag::UNLINK, _));
		EXPECT_CALL(comm, test).WillOnce(Return(true));
	}

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_distant_tgt_distant_src) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_tgt, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));
	ON_CALL(mock_tgt, location)
		.WillByDefault(Return(12));

	ON_CALL(mock_edge, state)
		.WillByDefault(Return(LocationState::DISTANT));
	{
		InSequence s;
		EXPECT_CALL(id_mpi, Issend(edge_id, 10, Epoch::EVEN | Tag::UNLINK, _));
		EXPECT_CALL(id_mpi, Issend(edge_id, 12, Epoch::EVEN | Tag::UNLINK, _));
		EXPECT_CALL(comm, test)
			.WillOnce(Return(true))
			.WillOnce(Return(true));
	}

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, remove_node) {
	ON_CALL(mock_src, state)
		.WillByDefault(Return(LocationState::DISTANT));
	ON_CALL(mock_src, location)
		.WillByDefault(Return(10));

	EXPECT_CALL(id_mpi, Issend(src_id, 10, Epoch::EVEN | Tag::REMOVE_NODE, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.removeNode(&mock_src);
}

class LinkClientDeadlockTest : public LinkClientTest {
	protected:
		Sequence s1_1, s1_2, s2_1, s2_2;

		void SetUp() override {
			LinkClientTest::SetUp();

			ON_CALL(mock_src, state)
				.WillByDefault(Return(LocationState::DISTANT));
			ON_CALL(mock_tgt, state)
				.WillByDefault(Return(LocationState::DISTANT));
			ON_CALL(mock_src, location)
				.WillByDefault(Return(10));
			ON_CALL(mock_tgt, location)
				.WillByDefault(Return(12));

			ON_CALL(mock_edge, state)
				.WillByDefault(Return(LocationState::DISTANT));
		}

		void waitS1() {
			// Each expectation must retire on saturation, so that when a test
			// call for example matches in S1, next calls will try to match in
			// S2 and not in the saturated S1 expectations
			for(int i = 0; i < 20; i++) {
				EXPECT_CALL(comm, test)
					.InSequence(s1_1, s1_2)
					.WillOnce(Return(false))
					.RetiresOnSaturation();
				EXPECT_CALL(mock_link_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s1_2)
					.RetiresOnSaturation();
				EXPECT_CALL(mock_mutex_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s1_1)
					.RetiresOnSaturation();
			}
			EXPECT_CALL(comm, test)
				.InSequence(s1_1, s1_2)
				.WillOnce(Return(true))
				.RetiresOnSaturation();
		}
		void waitS2() {
			for(int i = 0; i < 10; i++) {
				EXPECT_CALL(comm, test)
					.InSequence(s2_1, s2_2)
					.WillOnce(Return(false))
					.RetiresOnSaturation();
				EXPECT_CALL(mock_link_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s2_1)
					.RetiresOnSaturation();
				EXPECT_CALL(mock_mutex_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s2_2)
					.RetiresOnSaturation();
			}
			EXPECT_CALL(comm, test)
				.InSequence(s2_1, s2_2)
				.WillOnce(Return(true))
				.RetiresOnSaturation();
		}

		void waitExpectations() {
			waitS1();
			waitS2();
		}
};

TEST_F(LinkClientDeadlockTest, link) {
	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s1_1, s1_2);
	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 12, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s2_1, s2_2);

	waitExpectations();

	link_client.link(&mock_edge);
}

TEST_F(LinkClientDeadlockTest, unlink) {
	EXPECT_CALL(id_mpi, Issend(edge_id, 10, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s1_1, s1_2);
	EXPECT_CALL(id_mpi, Issend(edge_id, 12, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s2_1, s2_2);

	waitExpectations();

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientDeadlockTest, remove_node) {
	EXPECT_CALL(id_mpi, Issend(src_id, 10, Epoch::EVEN | Tag::REMOVE_NODE, _))
		.InSequence(s1_1, s1_2);

	waitS1();

	link_client.removeNode(&mock_src);
}
