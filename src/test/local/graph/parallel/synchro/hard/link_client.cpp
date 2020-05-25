#include "graph/parallel/synchro/hard/hard_sync_linker.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"
#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"

using ::testing::Sequence;
using ::testing::Ref;
using ::testing::Expectation;
using FPMAS::graph::parallel::synchro::hard::Tag;
using FPMAS::graph::parallel::synchro::hard::LinkClient;

class LinkClientTest : public ::testing::Test {
	protected:
		const DistributedId arcId {9, 2};
		MockMpiCommunicator<7, 16> comm;
		MockLinkServer mockLinkServer;
		LinkClient<
			MockDistributedArc<int, MockMutex>,
			MockMpi
			> linkClient {comm, mockLinkServer};

		MockDistributedNode<int, MockMutex> mockSrc;
		MockDistributedNode<int, MockMutex> mockTgt;

		MockDistributedArc<int, MockMutex> mockArc;

		void SetUp() override {
			ON_CALL(mockArc, getId)
				.WillByDefault(Return(arcId));
			EXPECT_CALL(mockArc, getId).Times(AnyNumber());

			ON_CALL(mockArc, getSourceNode)
				.WillByDefault(Return(&mockSrc));
			EXPECT_CALL(mockArc, getSourceNode).Times(AnyNumber());

			ON_CALL(mockArc, getTargetNode)
				.WillByDefault(Return(&mockTgt));
			EXPECT_CALL(mockArc, getTargetNode).Times(AnyNumber());

			ON_CALL(mockLinkServer, getEpoch).WillByDefault(Return(Epoch::EVEN));
			EXPECT_CALL(mockLinkServer, getEpoch).Times(AnyNumber());
		}
};

// Should not do anything
TEST_F(LinkClientTest, link_local_src_local_tgt) {
	EXPECT_CALL(mockSrc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockTgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(linkClient.getArcMpi(), Issend).Times(0);

	linkClient.link(&mockArc);
}

TEST_F(LinkClientTest, link_local_src_distant_tgt) {
	EXPECT_CALL(mockSrc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockTgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	linkClient.link(&mockArc);
}

TEST_F(LinkClientTest, link_local_tgt_distant_src) {
	EXPECT_CALL(mockSrc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockSrc, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	linkClient.link(&mockArc);
}

TEST_F(LinkClientTest, link_distant_tgt_distant_src) {
	EXPECT_CALL(mockSrc, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockSrc, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mockTgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 10, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 12, Epoch::EVEN | Tag::LINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	linkClient.link(&mockArc);
}

// Should not do anything
TEST_F(LinkClientTest, unlink_local_src_local_tgt) {
	EXPECT_CALL(mockSrc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockTgt, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::LOCAL));

	EXPECT_CALL(linkClient.getIdMpi(), Issend).Times(0);

	linkClient.unlink(&mockArc);
}

TEST_F(LinkClientTest, unlink_local_src_distant_tgt) {
	EXPECT_CALL(mockSrc, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockTgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	linkClient.unlink(&mockArc);
}

TEST_F(LinkClientTest, unlink_local_tgt_distant_src) {
	EXPECT_CALL(mockSrc, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, state).WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(mockSrc, getLocation).WillRepeatedly(Return(10));

	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test).WillOnce(Return(true));

	linkClient.unlink(&mockArc);
}

TEST_F(LinkClientTest, unlink_distant_tgt_distant_src) {
	EXPECT_CALL(mockSrc, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockTgt, state).WillRepeatedly(Return(LocationState::DISTANT));
	EXPECT_CALL(mockSrc, getLocation).WillRepeatedly(Return(10));
	EXPECT_CALL(mockTgt, getLocation).WillRepeatedly(Return(12));

	EXPECT_CALL(mockArc, state).Times(AnyNumber())
		.WillRepeatedly(Return(LocationState::DISTANT));

	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 12, Epoch::EVEN | Tag::UNLINK, _));
	EXPECT_CALL(comm, test)
		.WillOnce(Return(true))
		.WillOnce(Return(true));

	linkClient.unlink(&mockArc);
}

class LinkClientDeadlockTest : public LinkClientTest {
	protected:
		Sequence s1, s2;

		void SetUp() override {
			LinkClientTest::SetUp();

			EXPECT_CALL(mockSrc, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mockTgt, state).WillRepeatedly(Return(LocationState::DISTANT));
			EXPECT_CALL(mockSrc, getLocation).WillRepeatedly(Return(10));
			EXPECT_CALL(mockTgt, getLocation).WillRepeatedly(Return(12));

			EXPECT_CALL(mockArc, state).Times(AnyNumber())
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
				EXPECT_CALL(mockLinkServer, handleIncomingRequests)
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
				EXPECT_CALL(mockLinkServer, handleIncomingRequests)
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
	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 10, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s1);
	EXPECT_CALL(linkClient.getArcMpi(), Issend(Ref(mockArc), 12, Epoch::EVEN | Tag::LINK, _))
		.InSequence(s2);

	waitExpectations();

	linkClient.link(&mockArc);
}

TEST_F(LinkClientDeadlockTest, unlink) {
	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 10, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s1);
	EXPECT_CALL(linkClient.getIdMpi(), Issend(arcId, 12, Epoch::EVEN | Tag::UNLINK, _))
		.InSequence(s2);

	waitExpectations();

	linkClient.unlink(&mockArc);
}
