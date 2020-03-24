#include "gtest/gtest.h"
#include "graph/parallel/distributed_graph.h"
#include "utils/test.h"

#include "graph/parallel/synchro/hard_sync_mode.h"

using FPMAS::graph::base::DefaultLayer;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::modes::HardSyncMode;

/*
 * Illustrates a specific import case that caused a bug, because the same arc
 * was imported twice at the same proc.
 *
 * This happens notably in the following case, reproduced in this test :
 * - nodes 0 and 1 are linked and lives in distinct procs
 * - they are both imported to an other proc
 * - in consecuence, because each original proc does not know that the other
 *   node will also be imported to the destination proc, the arc connecting the
 *   two nodes (represented as a ghost arc on both original process) is
 *   imported twice to the destination proc.
 */
TEST(Mpi_DistributedGraphEdgeCases, duplicate_imported_arc_bug) {
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(size < 3){
		PRINT_MIN_PROCS_WARNING(duplicate_imported_arc_bug, 3)
			return;
	}
	DistributedGraph<int, HardSyncMode> dg;

	//DistributedGraph<int, HardSyncData> dg = DistributedGraph<int, HardSyncData>({0, 1, 2});
	if(dg.getMpiCommunicator().getRank() == 0) {
		auto localNode = dg.buildNode();
		dg.getProxy().setOrigin(DistributedId(1, 0), 1);
		dg.getProxy().setCurrentLocation(DistributedId(1, 0), 1);
		auto ghostNode = dg.getGhost().buildNode(DistributedId(1, 0));
		dg.getGhost().link(localNode, ghostNode, DistributedId(0, 0), DefaultLayer);
	}
	else if(dg.getMpiCommunicator().getRank() == 1) {
		auto localNode = dg.buildNode();
		dg.getProxy().setOrigin(DistributedId(0, 0), 0);
		dg.getProxy().setCurrentLocation(DistributedId(0, 0), 0);
		auto ghostNode = dg.getGhost().buildNode(DistributedId(0, 0));
		dg.getGhost().link(ghostNode, localNode, DistributedId(0, 0), DefaultLayer);
	}

	dg.distribute({
			{DistributedId(0, 0), std::pair(0, 2)},
			{DistributedId(1, 0), std::pair(1, 2)}
			});

	if(dg.getMpiCommunicator().getRank() == 2) {
		ASSERT_EQ(dg.getNodes().size(), 2);
		dg.removeNode(DistributedId(0, 0));

		// Checks that the node has been correctly removed with the
		// associated arc
		ASSERT_EQ(dg.getNodes().size(), 1);
		ASSERT_EQ(dg.getNodes().begin()->second->getIncomingArcs().size(), 0);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
	}
	dg.synchronize();
}
