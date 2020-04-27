#include "graph/parallel/synchro/basic_ghost_mode.h"

#include "api/communication/mock_communication.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/synchro/mock_mutex.h"

using FPMAS::graph::parallel::synchro::GhostSyncLinker;

TEST(GhostSyncLinker, link) {
	GhostSyncLinker<
		MockDistributedNode<int, MockMutex>,
		MockDistributedArc<int, MockMutex>,
		MockMpi>
		linker;

}
