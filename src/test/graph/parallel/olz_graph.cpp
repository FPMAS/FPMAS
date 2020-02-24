#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

class Mpi_SynchronizeGhostTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i);
					partition[i] = std::pair(0, i);
				}
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					// Build a ring across the processors
					dg.link(i, (i+1) % dg.getMpiCommunicator().getSize(), i);
				}
			}
			dg.distribute(partition);
			dg.getGhost().synchronize();
		}
};

TEST_F(Mpi_SynchronizeGhostTest, synchronize_ghost_test) {
	ASSERT_EQ(dg.getNodes().size(), 1);

	auto localNode = dg.getNodes().begin()->second;
	ASSERT_EQ(localNode->data()->get(), localNode->getId());
	
	// In a real scenario, data would be an object and some fields would be
	// updated, we won't recreate a complete object each time
	localNode->data()->acquire() = localNode->getId() + 1;
	localNode->setWeight(2.);

	dg.getGhost().synchronize();

	for(auto node : dg.getGhost().getNodes()) {
		ASSERT_EQ(node.second->data()->get(), node.first + 1);
		ASSERT_EQ(node.second->getWeight(), 2.);
	}
}
