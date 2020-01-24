#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/hard_sync_data.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::HardSyncData;


TEST(Mpi_HardSyncDistGraph, build_test) {
	DistributedGraph<int, HardSyncData> dg;
	dg.getGhost().buildNode(2);

}

class Mpi_HardSyncDistGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int, HardSyncData> dg = DistributedGraph<int, HardSyncData>();

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i);
				}
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					// Build a ring across the processors
					dg.link(i, (i+1) % dg.getMpiCommunicator().getSize(), i);
				}
			}
			dg.distribute();
		}
};

TEST_F(Mpi_HardSyncDistGraphTest, simple_read_test) {
	ASSERT_GE(dg.getGhost().getNodes().size(), 1);
	for(auto ghost : dg.getGhost().getNodes()) {
		ghost.second->data()->read();
		ASSERT_EQ(ghost.second->data()->read(), ghost.first);
	}
	dg.getMpiCommunicator().terminate();
};
