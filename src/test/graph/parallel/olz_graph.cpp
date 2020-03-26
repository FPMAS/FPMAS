#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::DistributedGraph;

class Mpi_SynchronizeGhostTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<DistributedId, std::pair<int, int>> partition;
		std::unordered_map<int, DistributedId> idMap;

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					idMap[i] = dg.buildNode(i, 0)->getId();
					partition[idMap[i]] = std::pair(0, i);
				}
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					// Build a ring across the processors
					dg.link(idMap[i], idMap[(i+1) % dg.getMpiCommunicator().getSize()]);
				}
			}
			dg.distribute(partition);
			dg.getGhost().synchronize();
		}
};

TEST_F(Mpi_SynchronizeGhostTest, synchronize_ghost_test) {
	ASSERT_EQ(dg.getNodes().size(), 1);

	auto localNode = dg.getNodes().begin()->second;
	ASSERT_EQ(localNode->data()->read(), localNode->getId().id());
	
	// In a real scenario, data would be an object and some fields would be
	// updated, we won't recreate a complete object each time
	localNode->data()->acquire() = localNode->getId().id() + 1;
	localNode->setWeight(2.);

	dg.getGhost().synchronize();

	for(auto node : dg.getGhost().getNodes()) {
		ASSERT_EQ(node.second->data()->read(), node.first.id() + 1);
		ASSERT_EQ(node.second->getWeight(), 2.);
	}
}
