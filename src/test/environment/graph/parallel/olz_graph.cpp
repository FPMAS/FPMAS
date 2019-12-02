#include "gtest/gtest.h"

#include "environment/graph/parallel/distributed_graph.h"

class Mpi_SynchronizeGhostTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();
		std::vector<int*> data;

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					data.push_back(new int(i));
					dg.buildNode(i, data.back());
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

	Node<int>* localNode = dg.getNodes().begin()->second;
	ASSERT_EQ(*localNode->getData(), localNode->getId());
	
	// In a real scenario, data would be an object and some fields would be
	// updated, we won't recreate a complete object each time
	int* newInt = new int(localNode->getId() + 1);
	localNode->setData(newInt);
	localNode->setWeight(2.);

	dg.getGhost()->synchronize();

	for(auto node : dg.getGhost()->getNodes()) {
		ASSERT_EQ(*node.second->getData(), node.first + 1);
		ASSERT_EQ(node.second->getWeight(), 2.);
	}

	delete newInt;
}
