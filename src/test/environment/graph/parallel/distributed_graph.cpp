#include "gtest/gtest.h"

#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::test_utils::assert_contains;

TEST(Mpi_DistributedGraph, build_with_ranks_test) {
	int global_size;
	MPI_Comm_size(MPI_COMM_WORLD, &global_size);
	int current_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);

	if(global_size == 1) {
		DistributedGraph<int> dg {current_rank};
		ASSERT_EQ(dg.getMpiCommunicator().getSize(), 1);
		ASSERT_EQ(dg.getMpiCommunicator().getRank(), current_rank);
	}
	else if(global_size >= 2) {
		if(!(global_size % 2 == 1 && current_rank == (global_size-1))) {
			int size;
			MPI_Comm_size(MPI_COMM_WORLD, &size);
			DistributedGraph<int>* dg;

			if(current_rank % 2 == 0) {
				dg = new DistributedGraph<int> {current_rank, (current_rank + 1) % size};
				ASSERT_EQ(dg->getMpiCommunicator().getRank(), 0);
			}
			else {
				dg = new DistributedGraph<int> {(size + current_rank - 1) % size, current_rank};
				ASSERT_EQ(dg->getMpiCommunicator().getRank(), 1);
			}
			ASSERT_EQ(dg->getMpiCommunicator().getSize(), 2);
			delete dg;
		}
		else {
			DistributedGraph<int> dg {current_rank};
			ASSERT_EQ(dg.getMpiCommunicator().getSize(), 1);
			ASSERT_EQ(dg.getMpiCommunicator().getRank(), 0);
		}
	}
}

TEST(Mpi_DistributedGraph, build_with_ranks_and_sync_mode_test) {
	int current_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);

	using FPMAS::graph::SyncMode::NONE;
	DistributedGraph<int> dg = DistributedGraph<int>({current_rank}, NONE);
	ASSERT_EQ(dg.getSyncMode(), NONE); 
}

class DistributeGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();
};

class Mpi_DistributeGraphWithoutArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode((unsigned long) i, i);
				}
			}
		}
};

TEST_F(Mpi_DistributeGraphWithoutArcTest, distribute_without_arc_test) {
	if(dg.getMpiCommunicator().getRank() == 0) {
		ASSERT_EQ(dg.getNodes().size(), dg.getMpiCommunicator().getSize());
	}
	else {
		ASSERT_EQ(dg.getNodes().size(), 0);
	}

	dg.distribute();

	ASSERT_EQ(dg.getNodes().size(), 1);

	// All nodes come from proc 0
	ASSERT_EQ(dg.getProxy()->getOrigin(
				dg.getNodes().begin()->first
				),
			0);

	// Proxy must return this proc as location for the local node
	ASSERT_EQ(dg.getProxy()->getCurrentLocation(
				dg.getNodes().begin()->first
				),
			dg.getMpiCommunicator().getRank()
			);

	// proc 0 must maintain the currentLocations map for exported nodes
	if(dg.getMpiCommunicator().getRank() == 0) {
		for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
			if(i == dg.getNodes().begin()->first) {
				// Local node on this proc
				ASSERT_EQ(dg.getProxy()->getCurrentLocation(i), 0);
			}
			else {
				// Must be located elsewhere
				ASSERT_NE(dg.getProxy()->getCurrentLocation(i), 0);
			}
		}

	}
}

class Mpi_DistributeGraphWithArcTest : public DistributeGraphTest {
	protected:

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode((unsigned long) 2 * i, 2*i);
					dg.buildNode((unsigned long) 2 * i + 1, 2*i);

					dg.link(2 * i, 2 * i + 1, i);
				}
			}
		}

};

TEST_F(Mpi_DistributeGraphWithArcTest, distribute_with_arc_test) {
	dg.distribute();

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getArcs().size(), 1);

	Arc<int>* arc = dg.getArcs().begin()->second;
	for(auto node : dg.getNodes()) {
		if(node.second->getIncomingArcs().size() == 1) {
			ASSERT_EQ(arc->getTargetNode()->getId(), node.second->getId());
		}
		else {
			ASSERT_EQ(node.second->getIncomingArcs().size(), 0);
			ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
			ASSERT_EQ(arc->getSourceNode()->getId(), node.second->getId());
		}
	}
}

class Mpi_DistributeGraphWithGhostArcsTest : public DistributeGraphTest {
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
	dg.distribute();
	ASSERT_EQ(dg.getNodes().size(), 1);

	// Must be performed with at least 2 procs
	if(dg.getMpiCommunicator().getSize() > 1) {
		ASSERT_EQ(dg.getArcs().size(), 0); 
		if(dg.getMpiCommunicator().getSize() == 2) {
			ASSERT_EQ(dg.getGhost()->getNodes().size(), 1);
		}
		else {
			ASSERT_EQ(dg.getGhost()->getNodes().size(), 2);
		}
		ASSERT_EQ(dg.getGhost()->getArcs().size(), 2);

		Node<int>* localNode = dg.getNodes().begin()->second;
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
}

TEST_F(Mpi_DistributeGraphWithGhostArcsTest, distribute_with_ghosts_proxy_test) {
	dg.distribute();
	ASSERT_EQ(dg.getNodes().size(), 1);

	int local_proc = dg.getMpiCommunicator().getRank();
	for(auto node : dg.getGhost()->getNodes()) {
		ASSERT_EQ(dg.getProxy()->getOrigin(node.first), 0);
		if(dg.getMpiCommunicator().getRank() != 0)
			ASSERT_NE(dg.getProxy()->getCurrentLocation(node.first), local_proc);
	}
}

class Mpi_DistributeCompleteGraphTest : public DistributeGraphTest {
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i);
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
TEST_F(Mpi_DistributeCompleteGraphTest, distribute_graph_with_multiple_ghost_arcs_test) {
	dg.distribute();

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(
		dg.getGhost()->getNodes().size(),
		2 * dg.getMpiCommunicator().getSize() - 2
		);

	for(auto ghostNode : dg.getGhost()->getNodes()) {
		ASSERT_EQ(ghostNode.second->getOutgoingArcs().size(), 2);
		ASSERT_EQ(ghostNode.second->getIncomingArcs().size(), 2);

		Node<int>* outNodes[2] = {
			ghostNode.second->getOutgoingArcs().at(0)->getTargetNode(),
			ghostNode.second->getOutgoingArcs().at(1)->getTargetNode()
		};
		Node<int>* inNodes[2] = {
			ghostNode.second->getIncomingArcs().at(0)->getSourceNode(),
			ghostNode.second->getIncomingArcs().at(1)->getSourceNode()
		};

		for(auto node : dg.getNodes()) {
			assert_contains<Node<int>*, 2>(outNodes, node.second);
			assert_contains<Node<int>*, 2>(inNodes, node.second);
		}

	}

}

TEST_F(Mpi_DistributeCompleteGraphTest, weight_load_balancing_test) {
	if(dg.getMpiCommunicator().getSize() % 2 == 0) {
		// Initial distribution
		dg.distribute();

		ASSERT_EQ(dg.getNodes().size(), 2);
		if(dg.getMpiCommunicator().getRank() % 2 == 0) {
			dg.getNodes().begin()->second->setWeight(3.);
		}
		
		dg.distribute();
		if(dg.getNodes().begin()->second->getWeight() == 3.) {
			ASSERT_EQ(dg.getNodes().size(), 1);
			ASSERT_EQ(dg.getNodes().begin()->second->getWeight(), 3.);
			ASSERT_EQ(dg.getGhost()->getNodes().size(), 2 * dg.getMpiCommunicator().getSize() - 1);
		}
		else {
			ASSERT_EQ(dg.getNodes().size(), 3);
			for(auto node : dg.getNodes()) {
				ASSERT_EQ(node.second->getWeight(), 1.);
			}
			ASSERT_EQ(dg.getGhost()->getNodes().size(), 2 * dg.getMpiCommunicator().getSize() - 3);
		}

		ASSERT_EQ(
			dg.getGhost()->getArcs().size(), 
			dg.getNodes().size() * 2 * dg.getGhost()->getNodes().size()
			);
	}
	else {
		PRINT_PAIR_PROCS_WARNING(weight_load_balancing_test);
	}

}

class Mpi_DistributeCompleteGraphTest_NoSync : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>(FPMAS::graph::SyncMode::NONE);

	void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode(i, i);
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
	dg.distribute();

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getArcs().size(), 2);

	for (auto node : dg.getNodes()) {
		ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
		ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
	}

	ASSERT_EQ(dg.getGhost()->getNodes().size(), 0);
	ASSERT_EQ(dg.getGhost()->getArcs().size(), 0);

}

class Mpi_DynamicLoadBalancingProxyTest : public DistributeGraphTest {

	void SetUp() override {
		if(dg.getMpiCommunicator().getRank() == 0) {
		for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
			dg.buildNode(i, i);
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

TEST_F(Mpi_DynamicLoadBalancingProxyTest, dynamic_lb_proxy_test) {
	// Initial distrib
	dg.distribute();
	dg.getProxy()->synchronize();
	ASSERT_EQ(dg.getNodes().size(), 2);

	// First round
	if(dg.getMpiCommunicator().getRank() % 2 == 1)
		dg.getNodes().begin()->second->setWeight(3.);

	dg.distribute();
	dg.getProxy()->synchronize();
	
	float totalWeight = 0.;
	for(auto node : dg.getNodes())
		totalWeight += node.second->getWeight();
	ASSERT_LE(totalWeight, 3.);

	// Second round
		for(auto node : dg.getNodes()) {
			if(node.second->getWeight() == 3.)
				node.second->setWeight(1.);
		}
	dg.distribute();
	// dg.getProxy()->synchronize();
	totalWeight = 0.;
	for(auto node : dg.getNodes())
		totalWeight += node.second->getWeight();
	ASSERT_LE(totalWeight, 2.);

}
