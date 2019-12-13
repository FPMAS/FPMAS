#include "gtest/gtest.h"

#include "environment/graph/parallel/distributed_graph.h"
#include "environment/graph/parallel/zoltan/zoltan_lb.h"
#include "utils/config.h"

#include "test_utils/test_utils.h"

using FPMAS::test_utils::assert_contains;

class DistributeGraphTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();
		std::vector<int*> data;
		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}
};

class Mpi_DistributeGraphWithoutArcTest : public DistributeGraphTest {
	protected:
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					data.push_back(new int(i));
					dg.buildNode((unsigned long) i, data.back());
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
					data.push_back(new int(2 * i));
					dg.buildNode((unsigned long) 2 * i, data.back());
					data.push_back(new int(2 * i + 1));
					dg.buildNode((unsigned long) 2 * i + 1, data.back());

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
					data.push_back(new int(i));
					dg.buildNode(i, data.back());
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
		PRINT_2_PROCS_WARNING(check_graph);
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
					data.push_back(new int(i));
					dg.buildNode(i, data.back());
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

		if(dg.getMpiCommunicator().getRank() % 2 == 0) {
			ASSERT_EQ(dg.getNodes().size(), 1);
			ASSERT_EQ(dg.getGhost()->getNodes().size(), 2 * dg.getMpiCommunicator().getSize() - 1);
		}
		else {
			ASSERT_EQ(dg.getNodes().size(), 3);
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

class Mpi_DynamicLoadBalancingProxyTest : public DistributeGraphTest {
	void SetUp() override {
		int numProcs = dg.getMpiCommunicator().getSize();
		// Creates numberProcs + 1 nodes, so that a proc will have two nodes
		if(dg.getMpiCommunicator().getRank() == 0) {
			for (int i = 0; i < numProcs + 1; ++i) {
				data.push_back(new int(i));
				dg.buildNode(i, data.back());
			}

			// We don't know which nodes will be on the same proc, so we link each
			// node to all others
			unsigned long arcId = 0;
			for (int i = 0; i < numProcs + 1; ++i) {
				for (int j = 0; j < numProcs + 1; ++j) {
					if(i != j)
						dg.link(i, j, arcId++);
				}
			}
		}
	}
};
/*
 *
 *TEST_F(Mpi_DynamicLoadBalancingProxyTest, dynamic_lb_proxy_test) {
 *    dg.distribute();
 *
 *    for(int i = 0; i < 2; i++) {
 *        bool procWithTwoNodes = false;
 *        std::cout << i << " " << dg.getMpiCommunicator().getRank() << " : " << dg.getNodes().size() << std::endl;
 *        if(dg.getNodes().size() == 2) {
 *            float total_weight = 0.;
 *            for(auto node : dg.getNodes())
 *                total_weight += node.second->getWeight();
 *
 *            ASSERT_NE(total_weight, 4.);
 *            procWithTwoNodes = true;
 *            // One of the nodes become too heavy
 *            dg.getNodes().begin()->second->setWeight(
 *                    3.
 *                    );
 *        }
 *        dg.distribute();
 *
 *        if(procWithTwoNodes) {
 *            ASSERT_EQ(dg.getNodes().size(), 1);
 *        }
 *    }
 *
 *}
 */
