#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/graph/mock_load_balancing.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/location_manager.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/api/graph/location_state.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "utils/test.h"

using fpmas::api::graph::LocationState;
using fpmas::graph::DistributedGraph;

using ::testing::AnyNumber;
using ::testing::ReturnRef;
using ::testing::SizeIs;

class DistributedGraphTest : public ::testing::Test {
	protected:
		fpmas::communication::MpiCommunicator comm;
		DistributedGraph<int, fpmas::synchro::GhostMode> graph {comm};
};

/*
 * Demonstrates how to use a temporary distant node and link it to a local
 * node.
 */
TEST_F(DistributedGraphTest, insertDistant) {
	FPMAS_MIN_PROCS("DistributedGraphTest.insertDistant", comm, 2) {
		auto local_node = graph.buildNode();

		fpmas::communication::TypedMpi<fpmas::graph::DistributedId> mpi(comm);
		auto node_id_map = mpi.migrate({{(comm.getRank() + 1) % comm.getSize(), {local_node->getId()}}});

		auto received_pair = node_id_map.begin();
		auto tmp_node = new fpmas::graph::DistributedNode<int>(*received_pair->second.begin(), 0);

		tmp_node->setLocation(received_pair->first);
		graph.insertDistant(tmp_node);

		graph.link(local_node, tmp_node, 0);
		graph.synchronize();

		ASSERT_THAT(local_node->getIncomingEdges(), SizeIs(1));
		ASSERT_THAT(local_node->getOutgoingEdges(), SizeIs(1));
	}
}

/****************************/
/* distribute_test_real_MPI */
/****************************/
/*
 * All the previous tests used locally mocked communications.
 * In those tests, a real MpiCommunicator and DistributedNode and
 * DistributedEdges implementations are used to distribute them accross the
 * available procs.
 */
class DistributedGraphBalance : public ::testing::Test {
	protected:

		fpmas::communication::MpiCommunicator comm;
		DistributedGraph<int, MockSyncMode> graph {comm};

		MockSyncLinker<int> mock_sync_linker;
		MockDataSync mock_data_sync;

		fpmas::api::graph::PartitionMap partition;

		void SetUp() override {
			EXPECT_CALL(mock_sync_linker, link).Times(AnyNumber());
			ON_CALL(graph.getSyncMode(), getSyncLinker)
				.WillByDefault(ReturnRef(mock_sync_linker));
			EXPECT_CALL(graph.getSyncMode(), getSyncLinker)
				.Times(AnyNumber());
			ON_CALL(graph.getSyncMode(), getDataSync)
				.WillByDefault(ReturnRef(mock_data_sync));
			EXPECT_CALL(graph.getSyncMode(), getDataSync)
				.Times(AnyNumber());

			FPMAS_ON_PROC(comm, 0) {
				EXPECT_CALL(graph.getSyncMode(), buildMutex)
					.Times(graph.getMpiCommunicator().getSize());
				auto firstNode = graph.buildNode();
				EXPECT_CALL(*dynamic_cast<MockMutex<int>*>(firstNode->mutex()), lockShared).Times(AnyNumber());
				EXPECT_CALL(*dynamic_cast<MockMutex<int>*>(firstNode->mutex()), unlockShared).Times(AnyNumber());

				auto prevNode = firstNode;
				partition[prevNode->getId()] = 0;
				for(auto i = 1; i < graph.getMpiCommunicator().getSize(); i++) {
					auto nextNode = graph.buildNode();
					EXPECT_CALL(*dynamic_cast<MockMutex<int>*>(nextNode->mutex()), lockShared).Times(AnyNumber());
					EXPECT_CALL(*dynamic_cast<MockMutex<int>*>(nextNode->mutex()), unlockShared).Times(AnyNumber());
					partition[nextNode->getId()] = i;
					graph.link(prevNode, nextNode, 0);
					prevNode = nextNode;
				}
				graph.link(prevNode, firstNode, 0);
			}
			else if(graph.getMpiCommunicator().getSize() == 2) {
				// 1 local node + 1 distant nodes will be created
				EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(2);
			} else {
				// 1 local node + 2 distant nodes will be created
				EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(3);
			}
		}
};

TEST_F(DistributedGraphBalance, distribute_test) {
	{
		::testing::InSequence s;
		EXPECT_CALL(mock_sync_linker, synchronize);
		EXPECT_CALL(mock_data_sync, synchronize);
	}
	graph.distribute(partition);

	if(graph.getMpiCommunicator().getSize() == 1) {
		ASSERT_EQ(graph.getNodes().size(), 1);
	}
	else if(graph.getMpiCommunicator().getSize() == 2) {
		ASSERT_EQ(graph.getNodes().size(), 2);
	}
	else {
		ASSERT_EQ(graph.getNodes().size(), 3);
	}
	ASSERT_THAT(graph.getLocationManager().getLocalNodes(), SizeIs(1));
	auto localNode = graph.getLocationManager().getLocalNodes().begin()->second;
	if(graph.getMpiCommunicator().getSize() > 1) {
		ASSERT_EQ(localNode->getOutgoingEdges().size(), 1);
		ASSERT_EQ(localNode->getOutgoingEdges()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getOutgoingEdges()[0]->getTargetNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingEdges().size(), 1);
		ASSERT_EQ(localNode->getIncomingEdges()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingEdges()[0]->getSourceNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(graph.getEdges().size(), 2);
	}
}
