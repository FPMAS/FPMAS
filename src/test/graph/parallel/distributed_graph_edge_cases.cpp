#include "gtest/gtest.h"
#include "graph/parallel/distributed_graph.h"
#include "test_utils/test_utils.h"

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
		dg.buildNode(0ul, 1);
		dg.getProxy().setOrigin(1ul, 0);
		dg.getProxy().setCurrentLocation(1ul, 1);
		dg.getGhost().buildNode(1ul);
		dg.getGhost().link(dg.getNodes().at(0ul), dg.getGhost().getNodes().at(1ul), 0ul, DefaultLayer);
	}
	else if(dg.getMpiCommunicator().getRank() == 1) {
		dg.buildNode(1ul, 1);
		dg.getProxy().setOrigin(0ul, 0);
		dg.getProxy().setCurrentLocation(0ul, 0);
		dg.getGhost().buildNode(0ul);
		dg.getGhost().link(dg.getGhost().getNodes().at(0ul), dg.getNodes().at(1ul), 0ul, DefaultLayer);
	}

	dg.distribute({
			{0ul, std::pair(0, 2)},
			{1ul, std::pair(1, 2)}
			});

	if(dg.getMpiCommunicator().getRank() == 2) {
		ASSERT_EQ(dg.getNodes().size(), 2);
		dg.getGhost().clear(dg.removeNode(0ul));

		// Checks that the node has been correctly removed with the
		// associated arc
		ASSERT_EQ(dg.getNodes().size(), 1);
		ASSERT_EQ(dg.getNodes().begin()->second->getIncomingArcs().size(), 0);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
	}
	dg.synchronize();
	/*
	 *else {
	 *    DistributedGraph<int, HardSyncData> dg = DistributedGraph<int, HardSyncData>({rank});
	 *}
	 */

}
