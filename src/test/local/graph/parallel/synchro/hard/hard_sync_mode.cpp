#include "graph/parallel/synchro/hard/hard_sync_mode.h"
#include "graph/parallel/basic_distributed_graph.h"
#include "graph/parallel/distributed_arc.h"
#include "graph/parallel/distributed_node.h"

#include "communication/communication.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"
#include "../mocks/graph/parallel/synchro/hard/mock_client_server.h"

using ::testing::Ref;

using FPMAS::graph::parallel::BasicDistributedGraph;
using FPMAS::graph::parallel::synchro::hard::HardSyncMode;
using FPMAS::graph::parallel::synchro::hard::HardDataSync;

class HardDataSyncTest : public ::testing::Test {
	protected:
		MockMpiCommunicator<2, 4> comm;
		MockMutexServer<int> mutexServer;
		HardDataSync<
			MockDistributedNode<int, MockMutex>,
			MockDistributedArc<int, MockMutex>,
			MockTerminationAlgorithm<int>> dataSync {comm, mutexServer};

};

TEST_F(HardDataSyncTest, synchronize) {
	EXPECT_CALL(
		const_cast<MockTerminationAlgorithm<int>&>(dataSync.getTerminationAlgorithm()),
		terminate(Ref(mutexServer))
		);
	dataSync.synchronize();
}

class HardSyncModeTest : public ::testing::Test {
	protected:
		BasicDistributedGraph<
			int, HardSyncMode,
			FPMAS::graph::parallel::DistributedNode,
			FPMAS::graph::parallel::DistributedArc,
			FPMAS::api::communication::MpiSetUp<
				MockMpiCommunicator<2, 12>,
				FPMAS::communication::TypedMpi
				>,
			MockLocationManager,
			MockLoadBalancing> graph;

};

TEST_F(HardSyncModeTest, test) {

}
