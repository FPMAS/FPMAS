#include "fpmas/synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_edge.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::Property;
using ::testing::Sequence;
using ::testing::Ref;
using ::testing::Expectation;
using fpmas::graph::EdgePtrWrapper;
using fpmas::synchro::hard::Tag;
using fpmas::synchro::hard::LinkClient;

class LinkClientTest : public ::testing::Test {
	protected:
		static const int THIS_RANK = 7;
		const DistributedId edgeId {9, 2};
		MockMpiCommunicator<THIS_RANK, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<EdgePtrWrapper<int>> edge_mpi {comm};

		MockTerminationAlgorithm mock_termination;
		MockMutexServer<int> mock_mutex_server;
		MockLinkServer mock_link_server;
		fpmas::synchro::hard::ServerPack<int> server_pack {
			comm, mock_termination, mock_mutex_server, mock_link_server};
		LinkClient<int> link_client {comm, id_mpi, edge_mpi, server_pack};

		MockDistributedNode<int> mock_src;
		MockDistributedNode<int> mock_tgt;

		MockDistributedEdge<int> mock_edge;

		void SetUp() override {
			ON_CALL(mock_edge, getId)
				.WillByDefault(Return(edgeId));
			EXPECT_CALL(mock_edge, getId).Times(AnyNumber());

			ON_CALL(mock_edge, getSourceNode)
				.WillByDefault(Return(&mock_src));
			EXPECT_CALL(mock_edge, getSourceNode).Times(AnyNumber());

			ON_CALL(mock_edge, getTargetNode)
				.WillByDefault(Return(&mock_tgt));
			EXPECT_CALL(mock_edge, getTargetNode).Times(AnyNumber());

			ON_CALL(mock_link_server, getEpoch).WillByDefault(Return(Epoch::EVEN));
			EXPECT_CALL(mock_link_server, getEpoch).Times(AnyNumber());
		}
};

// Should not do anything
TEST_F(LinkClientTest, link_local_src_local_tgt) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(edge_mpi, Issend).Times(0);

	link_client.link(&mock_edge);
}

TEST_F(LinkClientTest, link_local_src_distant_tgt) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_src, getLocation).Times(AnyNumber())
		.WillRepeatedly(Return(THIS_RANK));

	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_edge);
}

TEST_F(LinkClientTest, link_local_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_tgt, getLocation).Times(AnyNumber())
		.WillRepeatedly(Return(THIS_RANK));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_edge);
}

TEST_F(LinkClientTest, link_distant_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

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
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(edge_mpi, Issend(
				Property(&EdgePtrWrapper<int>::get, &mock_edge), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true));

	link_client.link(&mock_edge);
}

// Should not do anything
TEST_F(LinkClientTest, unlink_local_src_local_tgt) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(id_mpi, Issend).Times(0);

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_local_src_distant_tgt) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_src, getLocation).Times(AnyNumber())
		.WillRepeatedly(Return(THIS_RANK));

	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(edgeId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_local_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(edgeId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.unlink(&mock_edge);
}

TEST_F(LinkClientTest, unlink_distant_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mock_edge, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(edgeId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(id_mpi, Issend(edgeId, 12, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	link_client.unlink(&mock_edge);
}

class LinkClientDeadlockTest : public LinkClientTest {
	protected:
		Sequence s1_1, s1_2, s2_1, s2_2;

		void SetUp() override {
			LinkClientTest::SetUp();

			EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
			EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

			EXPECT_CALL(mock_edge, state).Times(AnyNumber())
				.WillRepeatedly(Return(LocationState::DISTANT));
		}

		void waitExpectations() {
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
	EXPECT_CALL(id_mpi, Issend(edgeId, 10, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s1_1, s1_2);
	EXPECT_CALL(id_mpi, Issend(edgeId, 12, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s2_1, s2_2);

	waitExpectations();

	link_client.unlink(&mock_edge);
}
