#include "gtest/gtest.h"

#include "test_utils/test_utils.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/sync_data.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::SyncDataPtr;

class Mpi_DistributeGraphWithGhostArcsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;
	
	public:
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
		}
};

TEST_F(Mpi_DistributeGraphWithGhostArcsTest, check_graph) {
	// This test must be performed with at least 2 procs
	if(dg.getMpiCommunicator().getSize() > 1) {
		if(dg.getMpiCommunicator().getRank() == 0) {
			ASSERT_EQ(dg.getNodes().size(), dg.getMpiCommunicator().getSize());
			for(auto node : dg.getNodes()) {
				ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
				ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
				if(dg.getMpiCommunicator().getSize() == 2) {
					// Two nodes with a bidirectionnal link
					ASSERT_EQ(
							node.second->getIncomingArcs().at(0)->getSourceNode(),
							node.second->getOutgoingArcs().at(0)->getTargetNode()
							);
				} else {
					// Real ring
					ASSERT_NE(
							node.second->getIncomingArcs().at(0)->getSourceNode(),
							node.second->getOutgoingArcs().at(0)->getTargetNode()
							);
				}
			}
		}
		else {
			ASSERT_EQ(dg.getNodes().size(), 0);
		}
	}
	else {
		PRINT_MIN_PROCS_WARNING(check_graph, 2);
	}

};

TEST_F(Mpi_DistributeGraphWithGhostArcsTest, distribute_with_ghosts_test) {
	dg.distribute(partition);
	ASSERT_EQ(dg.getNodes().size(), 1);

	// Must be performed with at least 2 procs
	if(dg.getMpiCommunicator().getSize() > 1) {
		ASSERT_EQ(dg.getArcs().size(), 0); 
		if(dg.getMpiCommunicator().getSize() == 2) {
			ASSERT_EQ(dg.getGhost().getNodes().size(), 1);
		}
		else {
			ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		}
		ASSERT_EQ(dg.getGhost().getArcs().size(), 2);

		auto localNode = dg.getNodes().begin()->second;
		ASSERT_EQ(localNode->getIncomingArcs().size(), 1);
		ASSERT_EQ(
				localNode->getIncomingArcs().at(0)->getSourceNode()->getId(),
				(dg.getMpiCommunicator().getSize() + localNode->getId() - 1) % dg.getMpiCommunicator().getSize()
				);
		ASSERT_EQ(localNode->getOutgoingArcs().size(), 1);
		ASSERT_EQ(
				localNode->getOutgoingArcs().at(0)->getTargetNode()->getId(),
				(localNode->getId() + 1) % dg.getMpiCommunicator().getSize()
				);
	}
	else {
		PRINT_MIN_PROCS_WARNING(check_graph, 2);
	}
}

TEST_F(Mpi_DistributeGraphWithGhostArcsTest, distribute_with_ghosts_proxy_test) {
	dg.distribute(partition);
	ASSERT_EQ(dg.getNodes().size(), 1);

	int local_proc = dg.getMpiCommunicator().getRank();
	for(auto node : dg.getGhost().getNodes()) {
		ASSERT_EQ(dg.getProxy().getOrigin(node.first), 0);
		if(dg.getMpiCommunicator().getRank() != 0)
			ASSERT_NE(dg.getProxy().getCurrentLocation(node.first), local_proc);
	}
}

class Mpi_DistributeCompleteGraphWithGhostTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;

	public:
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

/*
 * In the previous test, each ghost is connected to exactly one local node.
 * This test checks the ghost node creation when each node is connected to
 * multiple local nodes (2 in this case)
 */
TEST_F(Mpi_DistributeCompleteGraphWithGhostTest, distribute_graph_with_multiple_ghost_arcs_test) {
	dg.distribute(partition);

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(
		dg.getGhost().getNodes().size(),
		2 * dg.getMpiCommunicator().getSize() - 2
		);

	for(auto ghostNode : dg.getGhost().getNodes()) {
		ASSERT_EQ(ghostNode.second->getOutgoingArcs().size(), 2);
		ASSERT_EQ(ghostNode.second->getIncomingArcs().size(), 2);

		Node<SyncDataPtr<int, DefaultLayer, 1>, DefaultLayer, 1>* outNodes[2] = {
			ghostNode.second->getOutgoingArcs().at(0)->getTargetNode(),
			ghostNode.second->getOutgoingArcs().at(1)->getTargetNode()
		};
		Node<SyncDataPtr<int, DefaultLayer, 1>, DefaultLayer, 1>* inNodes[2] = {
			ghostNode.second->getIncomingArcs().at(0)->getSourceNode(),
			ghostNode.second->getIncomingArcs().at(1)->getSourceNode()
		};

		for(auto node : dg.getNodes()) {
			FPMAS::test_utils::assert_contains<Node<SyncDataPtr<int, DefaultLayer, 1>, DefaultLayer, 1>*, 2>(outNodes, node.second);
			FPMAS::test_utils::assert_contains<Node<SyncDataPtr<int, DefaultLayer, 1>, DefaultLayer, 1>*, 2>(inNodes, node.second);
		}

	}

}
