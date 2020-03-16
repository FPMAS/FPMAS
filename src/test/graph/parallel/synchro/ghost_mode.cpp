#include "gtest/gtest.h"

#include "utils/test.h"

#include "graph/parallel/distributed_graph.h"

using FPMAS::graph::parallel::synchro::wrappers::SyncData;
using FPMAS::graph::parallel::DistributedGraph;

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

		for(auto node : dg.getNodes()) {
			ASSERT_TRUE(
					ghostNode.second->getOutgoingArcs().at(0)->getTargetNode()
					== node.second
					||
					ghostNode.second->getOutgoingArcs().at(1)->getTargetNode()
					== node.second
					);
			ASSERT_TRUE(
					ghostNode.second->getIncomingArcs().at(0)->getSourceNode()
					== node.second
					||
					ghostNode.second->getIncomingArcs().at(1)->getSourceNode()
					== node.second
					);
		}
	}
}

class Mpi_DynamicLinkTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;
	
	public:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(2*i, 2*i);
					dg.buildNode(2*i+1, 2*i+1);
					partition[2*i] = std::pair(0, i);
					partition[2*i+1] = std::pair(0, i);
				}
				for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					dg.link(
						2*i+1,
						2*((i+1)%dg.getMpiCommunicator().getSize()),
						i
						);
				}
			}
		}
};

using FPMAS::graph::base::DefaultLayer;

TEST_F(Mpi_DynamicLinkTest, local_link) {
	if(dg.getMpiCommunicator().getSize() > 1) {
	dg.distribute(partition);

	int i = dg.getMpiCommunicator().getRank();
	dg.link(2*i, 2*i+1, dg.getMpiCommunicator().getSize() + i);

	ASSERT_EQ(dg.getArcs().size(), 1);
	const auto* node = dg.getNode(2*i);
	ASSERT_EQ(node->layer(DefaultLayer).getOutgoingArcs().size(), 1);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}

/*
 * i = current rank
 * On each proc, we have the following setup :
 * 	[2*i]<----(2*(i-1)+1)
 *
 * 	[2*i+1]-->(2*(i+1))
 * Then, we link the local node [2*i] to (2*(i+1)) so that, in the final setup,
 * we have 2 new arcs :
 * [2*i]------>(2*(i+1)) : arcs that need to be exported
 * (2*(i-1))-->[2*i]     : arc that should be imported
 *
 * Moreover, if N_proc > 2, the ghost node (2*(i-1)) must be created.
 *
 */
TEST_F(Mpi_DynamicLinkTest, ghost_target_link) {
	if(dg.getMpiCommunicator().getSize() > 1) {
		// Initial partitioning, 2 nodes per process
		// Even node has an incoming ghost
		// Odd node has an outgoing ghost
		dg.distribute(partition);

		int i = dg.getMpiCommunicator().getRank();
		const auto* node = dg.getNode(2*i);
		ASSERT_EQ(node->layer(DefaultLayer).getOutgoingArcs().size(), 0);
		ASSERT_EQ(node->layer(DefaultLayer).getIncomingArcs().size(), 1);

		ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().count(2*((i+1)%dg.getMpiCommunicator().getSize())), 1);
		// Links pair node to the other pair ghost node
		dg.link(
			2*i,
			2*((i+1)%dg.getMpiCommunicator().getSize()),
			dg.getMpiCommunicator().getSize() + i
			);
		// Migrates new link
		dg.synchronize();

		// Checks that the arc has locally been created
		ASSERT_EQ(node->layer(DefaultLayer).getOutgoingArcs().size(), 1);

		if(dg.getMpiCommunicator().getSize() > 2) {
			// Checks that the new ghost node has been created if necessary
			ASSERT_EQ(dg.getGhost().getNodes().size(), 3);
		} else {
			ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		}

		// Checks that an arc has been imported
		ASSERT_EQ(dg.getGhost().getArcs().size(), 4);
		ASSERT_EQ(node->layer(DefaultLayer).getIncomingArcs().size(), 2);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}

TEST_F(Mpi_DynamicLinkTest, ghost_source_link) {
	if(dg.getMpiCommunicator().getSize() > 1) {
		// Initial partitioning, 2 nodes per process
		// Even node has an incoming ghost
		// Odd node has an outgoing ghost
		dg.distribute(partition);

		int i = dg.getMpiCommunicator().getRank();
		const auto* node = dg.getNode(2*i);
		ASSERT_EQ(node->layer(DefaultLayer).getOutgoingArcs().size(), 0);
		ASSERT_EQ(node->layer(DefaultLayer).getIncomingArcs().size(), 1);

		ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().count(2*((i+1)%dg.getMpiCommunicator().getSize())), 1);
		// Links pair ghost to pair node
		dg.link(
			2*((i+1)%dg.getMpiCommunicator().getSize()),
			2*i,
			dg.getMpiCommunicator().getSize() + i
			);
		// Migrates new link
		dg.synchronize();

		// Checks that the arc has locally been created
		ASSERT_EQ(node->layer(DefaultLayer).getIncomingArcs().size(), 2);

		if(dg.getMpiCommunicator().getSize() > 2) {
			// Checks that the new ghost node has been created if necessary
			ASSERT_EQ(dg.getGhost().getNodes().size(), 3);
		} else {
			ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		}

		// Checks that an arc has been imported
		ASSERT_EQ(dg.getGhost().getArcs().size(), 4);
		ASSERT_EQ(node->layer(DefaultLayer).getOutgoingArcs().size(), 1);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}

class Mpi_DynamicGhostLinkTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<unsigned long, std::pair<int, int>> partition;
	
	public:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(3*i, 0);
					dg.buildNode(3*i+1, 0);
					dg.buildNode(3*i+2, 0);
					partition[3*i] = std::pair(0, i);
					partition[3*i+1] = std::pair(0, i);
					partition[3*i+2] = std::pair(0, i);
				}
				for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					dg.link(
						3*i+1,
						3*((i+1)%dg.getMpiCommunicator().getSize()),
						3*i
						);
					dg.link(
						3*((i+1)%dg.getMpiCommunicator().getSize())+2,
						3*i+1,
						3*i+1
						);
				}
			}
			dg.distribute(partition);
		}
};

TEST_F(Mpi_DynamicGhostLinkTest, ghost_source_and_target_link) {
	if(dg.getMpiCommunicator().getSize() >= 2) {
		// Original local graph structure
		ASSERT_EQ(dg.getNodes().size(), 3);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 3);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 4);

		// link 3(i+1) -> 3(i+2), that are two ghost nodes
		int i = dg.getMpiCommunicator().getRank();
		dg.link(
			3*((i+1)%dg.getMpiCommunicator().getSize()),
			3*((i+1)%dg.getMpiCommunicator().getSize())+2,
			3*((i+1)%dg.getMpiCommunicator().getSize())+2
			);
		// Checks that the created arc is a ghost arc
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 5);

		// Synchronize : the new local arc should be eported, and a distant arc
		// should be imported
		dg.synchronize();

		// Checks that local ghost data is not altered (no need to create new
		// ghost nodes, etc...)
		ASSERT_EQ(dg.getGhost().getNodes().size(), 3);
		// The GhostNode -> GhostNode link should be deleted
		ASSERT_EQ(dg.getGhost().getArcs().size(), 4);

		// Checks the imported arc structure
		ASSERT_EQ(dg.getArcs().size(), 1);
		ASSERT_EQ(dg.getNode(3*i)->getOutgoingArcs().size(), 1); // new arc
		ASSERT_EQ(dg.getNode(3*i)->getIncomingArcs().size(), 1); // ghost arc
		ASSERT_EQ(dg.getNode(3*i+2)->getIncomingArcs().size(), 1); // new arc
		ASSERT_EQ(dg.getNode(3*i+2)->getOutgoingArcs().size(), 1); // ghost arc
		ASSERT_EQ(
			dg.getNode(3*i)->getOutgoingArcs()[0]->getId(),
			dg.getNode(3*i+2)->getIncomingArcs()[0]->getId()
			);

		// Checks the imported arc id
		ASSERT_EQ(
			dg.getNode(3*i)->getOutgoingArcs()[0]->getId(),
			3*i+2
			);

	} else {
		PRINT_MIN_PROCS_WARNING(ghost_source_and_target_link, 2);
	}

}

/*
 * Distant unlink using arc id
 */
TEST_F(Mpi_DynamicLinkTest, unlink_with_id) {
	if(dg.getMpiCommunicator().getSize() > 1) {
		dg.distribute(partition);

		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 2);

		int i = dg.getMpiCommunicator().getRank();
		dg.unlink(i);

		ASSERT_EQ(dg.getGhost().getArcs().size(), 1);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 1);

		dg.synchronize();

		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
		ASSERT_EQ(dg.getArcs().size(), 0);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}

/*
 * Distant unlink using arc ptr
 */
TEST_F(Mpi_DynamicLinkTest, unlink_with_arc_ptr) {
	if(dg.getMpiCommunicator().getSize() > 1) {
		dg.distribute(partition);

		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 2);

		int i = dg.getMpiCommunicator().getRank();
		dg.unlink(*dg.getNode(2*i+1)->getOutgoingArcs().begin());

		ASSERT_EQ(dg.getGhost().getArcs().size(), 1);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 1);

		dg.synchronize();

		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
		ASSERT_EQ(dg.getArcs().size(), 0);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}

/*
 * The same arc might be unlinked from different procs in the same epoch.
 * In that case, the arc should be unlinked exactly once from other procs with
 * no error.
 */
TEST_F(Mpi_DynamicLinkTest, multiple_unlink) {
	if(dg.getMpiCommunicator().getSize() > 1) {
		dg.distribute(partition);

		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 2);
		ASSERT_EQ(dg.getArcs().size(), 0);
		ASSERT_EQ(dg.getGhost().getArcs().size(), 2);

		for(auto arc : dg.getGhost().getArcs()) {
			dg.unlink(arc.second);
		}

		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);

		dg.synchronize();

		ASSERT_EQ(dg.getGhost().getArcs().size(), 0);
		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getGhost().getNodes().size(), 0);
		ASSERT_EQ(dg.getArcs().size(), 0);
	} else {
		PRINT_MIN_PROCS_WARNING(ghost_target_link, 2);
	}
}
