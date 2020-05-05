
#include "../mocks/graph/parallel/mock_load_balancing.h"
#include "../mocks/graph/parallel/synchro/mock_sync_mode.h"
#include "communication/communication.h"
#include "graph/parallel/basic_distributed_graph.h"
#include "graph/parallel/location_manager.h"
#include "graph/parallel/distributed_node.h"
#include "graph/parallel/distributed_arc.h"
#include "api/graph/parallel/location_state.h"

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::graph::parallel::BasicDistributedGraph;

using ::testing::AnyNumber;
using ::testing::SizeIs;

/****************************/
/* distribute_test_real_MPI */
/****************************/
/*
 * All the previous tests used locally mocked communications.
 * In those tests, a real MpiCommunicator and DistributedNode and
 * DistributedArcs implementations are used to distribute them accross the
 * available procs.
 */
class Mpi_BasicDistributedGraphBalance : public ::testing::Test {
	protected:
		template<typename NodeType>
		using location_manager = FPMAS::graph::parallel::LocationManager<NodeType, FPMAS::communication::TypedMpi>;
		BasicDistributedGraph<
			int,
			MockSyncMode<>,
			FPMAS::graph::parallel::DistributedNode,
			FPMAS::graph::parallel::DistributedArc,
			FPMAS::api::communication::MpiSetUp<
				FPMAS::communication::MpiCommunicator,
				FPMAS::communication::TypedMpi
				>,
			location_manager,
			MockLoadBalancing> graph;

		MockLoadBalancing<FPMAS::graph::parallel::DistributedNode<int, MockMutex>>
			::partition_type partition;

		void SetUp() override {
			if(graph.getMpiCommunicator().getRank() == 0) {
				auto firstNode = graph.buildNode();
				EXPECT_CALL(firstNode->mutex(), lock).Times(AnyNumber());
				EXPECT_CALL(firstNode->mutex(), unlock).Times(AnyNumber());

				auto prevNode = firstNode;
				partition[prevNode->getId()] = 0;
				for(auto i = 1; i < graph.getMpiCommunicator().getSize(); i++) {
					auto nextNode = graph.buildNode();
					EXPECT_CALL(nextNode->mutex(), lock).Times(AnyNumber());
					EXPECT_CALL(nextNode->mutex(), unlock).Times(AnyNumber());
					partition[nextNode->getId()] = i;
					auto arc = graph.link(prevNode, nextNode, 0);
					prevNode = nextNode;
				}
				graph.link(prevNode, firstNode, 0);
			}
		}
};

TEST_F(Mpi_BasicDistributedGraphBalance, distribute_test) {
	EXPECT_CALL(graph.getSyncLinker(), synchronize);
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
		ASSERT_EQ(localNode->getOutgoingArcs().size(), 1);
		ASSERT_EQ(localNode->getOutgoingArcs()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getOutgoingArcs()[0]->getTargetNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingArcs().size(), 1);
		ASSERT_EQ(localNode->getIncomingArcs()[0]->state(), LocationState::DISTANT);
		ASSERT_EQ(localNode->getIncomingArcs()[0]->getSourceNode()->state(), LocationState::DISTANT);
		ASSERT_EQ(graph.getArcs().size(), 2);
	}
}
