#include "synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/synchro/mock_mutex.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using ::testing::Property;
using ::testing::Sequence;
using ::testing::Ref;
using ::testing::Expectation;
using fpmas::graph::parallel::ArcPtrWrapper;
using fpmas::synchro::hard::Tag;
using fpmas::synchro::hard::LinkClient;

class LinkClientTest : public ::testing::Test {
	protected:
		const DistributedId arcId {9, 2};
		MockMpiCommunicator<7, 16> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<ArcPtrWrapper<int>> arc_mpi {comm};

		MockLinkServer mock_link_server;
		LinkClient<int> link_client {comm, id_mpi, arc_mpi, mock_link_server};

		MockDistributedNode<int> mock_src;
		MockDistributedNode<int> mock_tgt;

		MockDistributedArc<int> mock_arc;

		void SetUp() override {
			ON_CALL(mock_arc, getId)
				.WillByDefault(Return(arcId));
			EXPECT_CALL(mock_arc, getId).Times(AnyNumber());

			ON_CALL(mock_arc, getSourceNode)
				.WillByDefault(Return(&mock_src));
			EXPECT_CALL(mock_arc, getSourceNode).Times(AnyNumber());

			ON_CALL(mock_arc, getTargetNode)
				.WillByDefault(Return(&mock_tgt));
			EXPECT_CALL(mock_arc, getTargetNode).Times(AnyNumber());

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
	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(arc_mpi, Issend).Times(0);

	link_client.link(&mock_arc);
}

TEST_F(LinkClientTest, link_local_src_distant_tgt) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(arc_mpi, Issend(Property(&ArcPtrWrapper<int>::get, &mock_arc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_arc);
}

TEST_F(LinkClientTest, link_local_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(arc_mpi, Issend(Property(&ArcPtrWrapper<int>::get, &mock_arc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.link(&mock_arc);
}

TEST_F(LinkClientTest, link_distant_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(arc_mpi, Issend(
				Property(&ArcPtrWrapper<int>::get, &mock_arc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(arc_mpi, Issend(
				Property(&ArcPtrWrapper<int>::get, &mock_arc), 12, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	link_client.link(&mock_arc);
}

// Should not do anything
TEST_F(LinkClientTest, unlink_local_src_local_tgt) {
	EXPECT_CALL(mock_src, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_tgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(id_mpi, Issend).Times(0);

	link_client.unlink(&mock_arc);
}

TEST_F(LinkClientTest, unlink_local_src_distant_tgt) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.unlink(&mock_arc);
}

TEST_F(LinkClientTest, unlink_local_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	link_client.unlink(&mock_arc);
}

TEST_F(LinkClientTest, unlink_distant_tgt_distant_src) {
	EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mock_arc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(id_mpi, Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(id_mpi, Issend(arcId, 12, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	link_client.unlink(&mock_arc);
}

class LinkClientDeadlockTest : public LinkClientTest {
	protected:
		Sequence s1, s2;

		void SetUp() override {
			LinkClientTest::SetUp();

			EXPECT_CALL(mock_src, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mock_tgt, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mock_src, getLocation).WillRepeatedly(Return(10));
			EXPECT_CALL(mock_tgt, getLocation).WillRepeatedly(Return(12));

			EXPECT_CALL(mock_arc, state).Times(AnyNumber())
				.WillRepeatedly(Return(LocationState::DISTANT));
		}

		void waitExpectations() {
			// Each expectation must retire on saturation, so that when a test
			// call for example matches in S1, next calls will try to match in
			// S2 and not in the saturated S1 expectations
			for(int i = 0; i < 20; i++) {
				EXPECT_CALL(comm, test)
					.InSequence(s1)
					.WillOnce(Return(false))
					.RetiresOnSaturation();
				EXPECT_CALL(mock_link_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s1)
					.RetiresOnSaturation();
			}
			EXPECT_CALL(comm, test)
				.InSequence(s1)
				.WillOnce(Return(true))
				.RetiresOnSaturation();

			for(int i = 0; i < 10; i++) {
				EXPECT_CALL(comm, test)
					.InSequence(s2)
					.WillOnce(Return(false))
					.RetiresOnSaturation();
				EXPECT_CALL(mock_link_server, handleIncomingRequests)
					.Times(AtLeast(1))
					.InSequence(s2)
					.RetiresOnSaturation();
			}
			EXPECT_CALL(comm, test)
				.InSequence(s2)
				.WillOnce(Return(true))
				.RetiresOnSaturation();
		}
};

TEST_F(LinkClientDeadlockTest, link) {
	EXPECT_CALL(arc_mpi, Issend(Property(&ArcPtrWrapper<int>::get, &mock_arc), 10, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s1);
	EXPECT_CALL(arc_mpi, Issend(Property(&ArcPtrWrapper<int>::get, &mock_arc), 12, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s2);

	waitExpectations();

	link_client.link(&mock_arc);
}

TEST_F(LinkClientDeadlockTest, unlink) {
	EXPECT_CALL(id_mpi, Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s1);
	EXPECT_CALL(id_mpi, Issend(arcId, 12, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s2);

	waitExpectations();

	link_client.unlink(&mock_arc);
}
