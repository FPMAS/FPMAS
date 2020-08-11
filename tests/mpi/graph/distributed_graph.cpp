#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/location_manager.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/graph/distributed_edge.h"
#include "fpmas/api/graph/location_state.h"

using fpmas::api::graph::LocationState;
using fpmas::graph::DistributedGraph;

using ::testing::AnyNumber;
using ::testing::ReturnRef;
using ::testing::SizeIs;

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
		template<typename T>
		using location_manager = fpmas::graph::LocationManager<T>;
		DistributedGraph<
			int,
			MockSyncMode,
			fpmas::graph::DistributedNode,
			fpmas::graph::DistributedEdge,
			fpmas::api::communication::MpiSetUp<
				fpmas::communication::MpiCommunicator,
				fpmas::communication::TypedMpi
				>,
			location_manager
			> graph;

		MockSyncLinker<int> mock_sync_linker;
		MockDataSync mock_data_sync;

		fpmas::api::load_balancing::PartitionMap partition;

		void SetUp() override {
			EXPECT_CALL(mock_sync_linker, link).Times(AnyNumber());
			ON_CALL(graph.getSyncModeRuntime(), getSyncLinker)
				.WillByDefault(ReturnRef(mock_sync_linker));
			EXPECT_CALL(graph.getSyncModeRuntime(), getSyncLinker)
				.Times(AnyNumber());
			ON_CALL(graph.getSyncModeRuntime(), getDataSync)
				.WillByDefault(ReturnRef(mock_data_sync));
			EXPECT_CALL(graph.getSyncModeRuntime(), getDataSync)
				.Times(AnyNumber());

			if(graph.getMpiCommunicator().getRank() == 0) {
				EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex)
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
				EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(2);
			} else {
				// 1 local node + 2 distant nodes will be created
				EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(3);
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
