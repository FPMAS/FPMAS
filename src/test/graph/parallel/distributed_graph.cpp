#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "utils/test.h"

using FPMAS::graph::parallel::synchro::modes::GhostMode;

using FPMAS::graph::parallel::DistributedGraph;

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

	using FPMAS::graph::parallel::synchro::modes::NoSyncMode;
	DistributedGraph<int, NoSyncMode> dg = DistributedGraph<int, NoSyncMode>({current_rank});

	// TODO: Assert idea?
}

class DistributeGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;
		std::unordered_map<int, DistributedId> idMap;
};

class Mpi_DistributeGraphWithoutArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					idMap[i] = dg.buildNode(i, 0)->getId();
				}
			}
		}
};

TEST_F(Mpi_DistributeGraphWithoutArcTest, distribute_without_arc_test_manual_partition) {
	std::unordered_map<DistributedId, int> partition;
	if(dg.getMpiCommunicator().getRank() == 0) {
		for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
			partition[idMap[i]] = i;
		}
	}
	dg.distribute(partition);
	ASSERT_EQ(dg.getNodes().size(), 1);
	ASSERT_EQ(dg.getNodes().begin()->first.id(), dg.getMpiCommunicator().getRank());
	ASSERT_EQ(dg.getNodes().begin()->second->getId().id(), dg.getMpiCommunicator().getRank());
	ASSERT_EQ(dg.getNodes().begin()->second->data()->read(), dg.getMpiCommunicator().getRank());
}

TEST_F(Mpi_DistributeGraphWithoutArcTest, distribute_without_arc_test) {
	if(dg.getMpiCommunicator().getRank() == 0) {
		ASSERT_EQ(dg.getNodes().size(), dg.getMpiCommunicator().getSize());
	} else {
		ASSERT_EQ(dg.getNodes().size(), 0);
	}

	dg.distribute();

	if(dg.getMpiCommunicator().getSize() > 1)
		ASSERT_LT(dg.getNodes().size(), dg.getMpiCommunicator().getSize());

	// Proxy must return this proc as location for the local node
	for(auto node : dg.getNodes()) {
		// All nodes come from proc 0
		ASSERT_EQ(dg.getProxy().getOrigin(
					dg.getNodes().begin()->first
					),
				0);
		ASSERT_EQ(dg.getProxy().getCurrentLocation(
					node.first
					),
				dg.getMpiCommunicator().getRank()
				);
	}

	// proc 0 must maintain the currentLocations map for exported nodes
	if(dg.getMpiCommunicator().getRank() == 0) {
		for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
			if(dg.getNodes().count(idMap[i]) > 0) {
				// Local node on this proc
				ASSERT_EQ(dg.getProxy().getCurrentLocation(idMap[i]), 0);
			}
			else {
				// Must be located elsewhere
				ASSERT_NE(dg.getProxy().getCurrentLocation(idMap[i]), 0);
			}
		}

	}
}

class Mpi_DistributeGraphWithArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					idMap[2*i] = dg.buildNode()->getId();
					idMap[2*i+1] = dg.buildNode()->getId();

					dg.link(idMap[2*i], idMap[2*i+1]);
				}
			}
		}

};

/*
 * Forces partition with 2 connected nodes by proc
 */
TEST_F(Mpi_DistributeGraphWithArcTest, distribute_with_arc_test_manual_partition) {
	std::unordered_map<DistributedId, int> partition;
	if(dg.getMpiCommunicator().getRank()==0) {
		for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
			partition[idMap[2*i]] = i;
			partition[idMap[2*i+1]] = i;
		}
	}
	dg.distribute(partition);

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getArcs().size(), 1);

	auto arc = dg.getArcs().begin()->second;
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

};

/*
 * Relaxed distribution.
 * The fact that the graph structure is saved is checked by the previous test.
 */
TEST_F(Mpi_DistributeGraphWithArcTest, distribute_with_arc_test) {
	dg.distribute();

	if(dg.getMpiCommunicator().getSize() > 1)
		// Assert that at least some nodes have been migrated.
		ASSERT_LT(dg.getNodes().size(), 2 * dg.getMpiCommunicator().getSize());
}

class Mpi_DistributeCompleteGraphTest : public DistributeGraphTest {
	protected:
		std::unordered_map<DistributedId, int> init_partition;
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					idMap[i] = dg.buildNode()->getId();
					init_partition[idMap[i]] = i / 2;
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


/*
 * Assert that modifying nodes weight updates partitions
 */
TEST_F(Mpi_DistributeCompleteGraphTest, weight_load_balancing_test) {
	if(dg.getMpiCommunicator().getSize() % 2 == 0) {
		// Initial distribution
		dg.distribute(init_partition);

		ASSERT_EQ(dg.getNodes().size(), 2);
		if(dg.getMpiCommunicator().getRank() % 2 == 0) {
			dg.getNodes().begin()->second->setWeight(3.);
		}
		
		dg.distribute();

		ASSERT_EQ(
			dg.getGhost().getArcs().size(), 
			dg.getNodes().size() * 2 * dg.getGhost().getNodes().size()
			);
	}
	else {
		PRINT_PAIR_PROCS_WARNING(weight_load_balancing_test);
	}

}

class Mpi_DynamicLoadBalancingTest : public DistributeGraphTest {

	protected:
		std::unordered_map<DistributedId, int> init_partition;
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 2 * dg.getMpiCommunicator().getSize(); ++i) {
					idMap[i] = dg.buildNode()->getId();
					init_partition[idMap[i]] = i / 2;
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

/*
 * Assert that load balancing functions can be called multiple times
 * while weights are updated
 * (dynamic load balancing)
 */
TEST_F(Mpi_DynamicLoadBalancingTest, dynamic_lb_test) {
	// Initial distrib
	dg.distribute(init_partition);
	ASSERT_EQ(dg.getNodes().size(), 2);

	// First round
	if(dg.getMpiCommunicator().getRank() % 2 == 1)
		dg.getNodes().begin()->second->setWeight(3.);

	dg.distribute();
	
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
	totalWeight = 0.;
	for(auto node : dg.getNodes())
		totalWeight += node.second->getWeight();
	ASSERT_LE(totalWeight, 2.);
}

class Mpi_ScheduledDistributionFromZeroTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;

		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					dg.buildNode(0, 0);
					dg.buildNode(0, 5);
				}
			}
		}
};

TEST_F(Mpi_ScheduledDistributionFromZeroTest, distribute) {
	if(dg.getMpiCommunicator().getSize()%2==0) {
		dg.distribute();

		ASSERT_EQ(dg.getScheduler().get(0).size(), 1);
		ASSERT_EQ(dg.getScheduler().get(5).size(), 1);
	} else {
		PRINT_PAIR_PROCS_WARNING(Mpi_ScheduledDistributionFromZeroTest.distribute);
	}
}

class Mpi_ScheduledDistributionFromZeroWithLinksTest : public Mpi_ScheduledDistributionFromZeroTest {
	protected:
		void SetUp() override {
			Mpi_ScheduledDistributionFromZeroTest::SetUp();
			if(dg.getMpiCommunicator().getRank() == 0) {
				auto nodes = dg.getScheduler().get(0);
				auto previous = *nodes.rbegin();
				for(auto node : nodes) {
					dg.link(previous, node);
					previous = node;
				}

				nodes = dg.getScheduler().get(5);
				previous = *nodes.rbegin();
				for(auto node : nodes) {
					dg.link(previous, node);
					previous = node;
				}
			}
		}
};

TEST_F(Mpi_ScheduledDistributionFromZeroWithLinksTest, distribute) {
	if(dg.getMpiCommunicator().getSize()%2==0) {
		for(auto node : dg.getNodes()) {
			ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
			ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
		}

		dg.distribute();

		ASSERT_EQ(dg.getNodes().size(), 2);
		ASSERT_EQ(dg.getScheduler().get(0).size(), 1);
		ASSERT_EQ(dg.getScheduler().get(5).size(), 1);

		for(auto node : dg.getNodes()) {
			ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
			ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
		}
	} else {
		PRINT_PAIR_PROCS_WARNING(Mpi_ScheduledDistributionFromZeroWithLinksTest.distribute);
	}
}

class Mpi_ScheduledDistributionFromAllProcsTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg;

		void SetUp() override {
			std::unordered_map<DistributedId, int> partition;
			int size = dg.getMpiCommunicator().getSize();
			if(size%2==0) {
				if(dg.getMpiCommunicator().getRank() == 0) {
					for (int i = 0; i < size; i++) {
						if(i%2==0) {
							partition[dg.buildNode(0, 0)->getId()] = i;
							partition[dg.buildNode(0, 0)->getId()] = i;
						} else {
							partition[dg.buildNode(0, 5)->getId()] = i;
							partition[dg.buildNode(0, 5)->getId()] = i;
						}
					}
					auto nodes = dg.getScheduler().get(0);
					auto previous = *nodes.rbegin();
					for(auto node : nodes) {
						dg.link(previous, node);
						previous = node;
					}

					nodes = dg.getScheduler().get(5);
					previous = *nodes.rbegin();
					for(auto node : nodes) {
						dg.link(previous, node);
						previous = node;
					}
				}
				dg.distribute(partition);
			}
		}
};

TEST_F(Mpi_ScheduledDistributionFromAllProcsTest, distribute) {
	if(dg.getMpiCommunicator().getSize()%2==0) {
		ASSERT_EQ(dg.getNodes().size(), 2);
		if(dg.getMpiCommunicator().getRank()%2==0) {
			ASSERT_EQ(dg.getScheduler().get(0).size(), 2);
		} else {
			ASSERT_EQ(dg.getScheduler().get(5).size(), 2);
		}
		dg.distribute();

		ASSERT_EQ(dg.getScheduler().get(0).size(), 1);
		ASSERT_EQ(dg.getScheduler().get(5).size(), 1);

		for(auto node : dg.getNodes()) {
			ASSERT_EQ(node.second->getIncomingArcs().size(), 1);
			ASSERT_EQ(node.second->getOutgoingArcs().size(), 1);
		}
	} else {
		PRINT_PAIR_PROCS_WARNING(Mpi_ScheduledDistributionFromAllProcsTest.distribute);
	}

}
