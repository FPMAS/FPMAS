#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::graph::parallel::synchro::None;

class Mpi_DistributeCompleteGraphTest_NoSync : public ::testing::Test {
	protected:
		DistributedGraph<int, None> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;

	void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i);
					partition[i] = std::pair(0, i / 2);
				}
				int id = 0;
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					for (int j = 0; j < 2 * dg.getMpiCommunicator().getSize(); ++j) {
						if(i != j) {
							dg.link(i, j, id++);
						}
					}
				}
			}
		}
};

TEST_F(Mpi_DistributeCompleteGraphTest_NoSync, no_sync_distribution) {
	dg.distribute(partition);

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getArcs().size(), 2);

	for (auto node : dg.getNodes()) {
		ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
		ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
	}

	ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
	ASSERT_EQ(dg.getGhost().getArcs().size(), 0);

}
