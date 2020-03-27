#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::graph::parallel::synchro::modes::NoSyncMode;

class Mpi_DistributeCompleteGraphTest_NoSync : public ::testing::Test {
	protected:
		DistributedGraph<int, NoSyncMode> dg;
		std::unordered_map<DistributedId, int> partition;

	void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				std::unordered_map<int, DistributedId> idMap;
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					auto node = dg.buildNode();
					partition[node->getId()] = i / 2;
					idMap[i] = node->getId();
				}
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					for (int j = 0; j < 2 * dg.getMpiCommunicator().getSize(); ++j) {
						if(i != j) {
							dg.link(idMap[i], idMap[j]);
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
