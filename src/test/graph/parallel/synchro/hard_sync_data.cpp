#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/hard_sync_data.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::HardSyncData;

TEST(Mpi_HardSyncData, build_test) {

}

TEST(Mpi_HardSyncDistGraph, build_test) {
	DistributedGraph<int, HardSyncData> dg;
	dg.getGhost().buildNode(2);

}
