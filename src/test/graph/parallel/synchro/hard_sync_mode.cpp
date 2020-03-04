#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/hard_sync_mode.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::HardSyncMode;


TEST(Mpi_HardSyncDistGraph, build_test) {
	DistributedGraph<int, HardSyncMode> dg;
	dg.getGhost().buildNode(2);

}

class Mpi_HardSyncDistGraphReadTest : public ::testing::Test {
	protected:
		DistributedGraph<int, HardSyncMode> dg;
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
		}
};

TEST_F(Mpi_HardSyncDistGraphReadTest, simple_read_test) {
	if(dg.getMpiCommunicator().getSize() > 1)
		ASSERT_GE(dg.getGhost().getNodes().size(), 1);

	for(auto ghost : dg.getGhost().getNodes()) {
		ghost.second->data()->read();
		ASSERT_EQ(ghost.second->data()->read(), ghost.first);
	}
	dg.synchronize();
};

class Mpi_HardSyncDistGraphAcquireTest : public ::testing::Test {
	protected:
		DistributedGraph<int, HardSyncMode> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				// Builds N node
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i, 0);
					partition[i] = std::pair(0, i);
				}
				// Each node is connected to node N-1, except N-1 itself
				for (int i = 0; i < dg.getMpiCommunicator().getSize() - 1; ++i) {
					if(i != dg.getMpiCommunicator().getSize() - 1)
						dg.link(i, dg.getMpiCommunicator().getSize() - 1, i);
				}
			}
			dg.distribute(partition);
		}
};

/*
 * In this test, all the procs, except N-1, concurrently adds 1 to the value of
 * the node N-1, so that the final sum on node N-1 must be N-1. The test
 * asserts that each proc as an exclusive access to N-1, to prevent race
 * conditions.
 */
TEST_F(Mpi_HardSyncDistGraphAcquireTest, race_condition_test) {
	// We don't know which node is on which process, but it doesn't matter
	for(auto node : dg.getNodes()) {
		// Actually, each node has 0 or 1 outgoing arc
		for(auto arc : node.second->getOutgoingArcs()) {
			// It is important to get the REFERENCE to the internal data
			int& data = arc->getTargetNode()->data()->acquire();
			// Updates the referenced data
			data++;
			// Gives updates back
			arc->getTargetNode()->data()->release();
		}
	}
	dg.synchronize();

	// Node where the sum is stored
	int sum_node = dg.getMpiCommunicator().getSize() - 1;
	// Sum value
	unsigned long sum = dg.getMpiCommunicator().getSize() - 1;
	// Looking for the graph (ie the proc) where N-1 is located
	if(dg.getNodes().count(sum_node)) {
		// Asserts the sum has been performed correctly, and that local data is
		// updated.
		ASSERT_EQ(
				dg.getNodes().at(sum_node)->data()->read(),
				sum
				);
	}
}

/*
 * Same as before, but with 500 acquires by proc
 */
TEST_F(Mpi_HardSyncDistGraphAcquireTest, heavy_race_condition_test) {
	for(auto node : dg.getNodes()) {
		// Actually, each node has 0 or 1 outgoing arc
		for(auto arc : node.second->getOutgoingArcs()) {
			for (int i = 0; i < 500; ++i) {
				// It is important to get the REFERENCE to the internal data
				int& data = arc->getTargetNode()->data()->acquire();
				// Updates the referenced data
				data++;
				// Gives updates back
				arc->getTargetNode()->data()->release();
			}
		}
	}
	dg.synchronize();

	// Node where the sum is stored
	unsigned long sum_node = dg.getMpiCommunicator().getSize() - 1;
	// Sum value
	unsigned long sum = 500 * (dg.getMpiCommunicator().getSize() - 1);
	// Looking for the graph (ie the proc) where N-1 is located
	if(dg.getNodes().count(sum_node)) {
		// Asserts the sum has been performed correctly, and that local data is
		// updated.
		ASSERT_EQ(
				dg.getNodes().at(sum_node)->data()->read(),
				sum
				);
	}
}
