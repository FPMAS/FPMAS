#include "gtest/gtest.h"

#include "environment/graph/parallel/distributed_graph.h"

using FPMAS::graph::DistributedGraph;
using FPMAS::graph::synchro::SyncData;

class Mpi_SynchronizeGhostTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

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
			dg.getGhost()->synchronize();
		}
};

TEST_F(Mpi_SynchronizeGhostTest, synchronize_ghost_test) {
	ASSERT_EQ(dg.getNodes().size(), 1);

	Node<SyncData<int>>* localNode = dg.getNodes().begin()->second;
	ASSERT_EQ(localNode->data().get(), localNode->getId());
	
	// In a real scenario, data would be an object and some fields would be
	// updated, we won't recreate a complete object each time
	localNode->data().get() = localNode->getId() + 1;
	localNode->setWeight(2.);

	dg.getGhost()->synchronize();

	for(auto node : dg.getGhost()->getNodes()) {
		ASSERT_EQ(node.second->data().get(), node.first + 1);
		ASSERT_EQ(node.second->getWeight(), 2.);
	}
}
