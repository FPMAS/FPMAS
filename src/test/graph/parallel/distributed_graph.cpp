#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::test_utils::assert_contains;
using FPMAS::graph::parallel::synchro::GhostData;

using FPMAS::graph::parallel::DistributedGraph;

using FPMAS::graph::parallel::synchro::SyncData;

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

	using FPMAS::graph::parallel::synchro::None;
	DistributedGraph<int, None> dg = DistributedGraph<int, None>({current_rank});

	// TODO: Assert idea?
}

class DistributeGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();
};

class Mpi_DistributeGraphWithoutArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
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
	ASSERT_EQ(dg.getProxy().getOrigin(
				dg.getNodes().begin()->first
				),
			0);

	// Proxy must return this proc as location for the local node
	ASSERT_EQ(dg.getProxy().getCurrentLocation(
				dg.getNodes().begin()->first
				),
			dg.getMpiCommunicator().getRank()
			);

	// proc 0 must maintain the currentLocations map for exported nodes
	if(dg.getMpiCommunicator().getRank() == 0) {
		for(int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
			if(i == dg.getNodes().begin()->first) {
				// Local node on this proc
				ASSERT_EQ(dg.getProxy().getCurrentLocation(i), 0);
			}
			else {
				// Must be located elsewhere
				ASSERT_NE(dg.getProxy().getCurrentLocation(i), 0);
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

	Arc<SyncData<int>>* arc = dg.getArcs().begin()->second;
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

class Mpi_DistributeCompleteGraphTest : public DistributeGraphTest {
	protected:
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
			ASSERT_EQ(dg.getGhost().getNodes().size(), 2 * dg.getMpiCommunicator().getSize() - 1);
		}
		else {
			ASSERT_EQ(dg.getNodes().size(), 3);
			for(auto node : dg.getNodes()) {
				ASSERT_EQ(node.second->getWeight(), 1.);
			}
			ASSERT_EQ(dg.getGhost().getNodes().size(), 2 * dg.getMpiCommunicator().getSize() - 3);
		}

		ASSERT_EQ(
			dg.getGhost().getArcs().size(), 
			dg.getNodes().size() * 2 * dg.getGhost().getNodes().size()
			);
	}
	else {
		PRINT_PAIR_PROCS_WARNING(weight_load_balancing_test);
	}

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
