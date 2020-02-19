#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"
#include "test_utils/test_utils.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::SyncDataPtr;
using FPMAS::graph::parallel::synchro::GhostData;

enum TestLayer {
	DEFAULT = 0,
	TEST = 1
};


class Mpi_DistributeMultiGraphWithArcTest : public ::testing::Test {
	protected:
		DistributedGraph<int, GhostData, TestLayer, 2> dg;
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
					dg.buildNode((unsigned long) 2 * i, 2*i);
					dg.buildNode((unsigned long) 2 * i + 1, 2*i);

					if(i % 2 == 0) {
						dg.link(2 * i, 2 * i + 1, i);
					} else {
						dg.link(2 * i, 2 * i + 1, i, TestLayer::TEST);
					}
				}
			}
		}

};

TEST_F(Mpi_DistributeMultiGraphWithArcTest, distribute_with_arc_test) {
	dg.distribute();

	ASSERT_LT(dg.getNodes().size(), 2 * dg.getMpiCommunicator().getSize());

	for(auto node : dg.getNodes()) {
		const Node<SyncDataPtr<int, TestLayer, 2>, TestLayer, 2>* node_ptr = node.second;
		if(node.first % 2 == 0) {
			// Source node
			if((node.first / 2) % 2 == 0) {
				// i pair => default layer
				ASSERT_EQ(node_ptr->getOutgoingArcs().size(), 1);
				ASSERT_EQ(node_ptr->layer(TEST).getOutgoingArcs().size(), 0);
				ASSERT_EQ(node_ptr->getOutgoingArcs()[0]->getTargetNode()->getId(), node_ptr->getId()+1);
			} else {
				// i odd => test layer
				ASSERT_EQ(node_ptr->layer(TEST).getOutgoingArcs().size(), 1);
				ASSERT_EQ(node_ptr->getOutgoingArcs().size(), 0);
				ASSERT_EQ(node_ptr->layer(TEST).getOutgoingArcs()[0]->getTargetNode()->getId(), node_ptr->getId()+1);
			}
		}
		else {
			// Target node
			if(((node.first - 1) / 2) % 2 == 0) {
				// i pair => default layer
				ASSERT_EQ(node_ptr->getIncomingArcs().size(), 1);
				ASSERT_EQ(node_ptr->layer(TEST).getIncomingArcs().size(), 0);
				ASSERT_EQ(node_ptr->getIncomingArcs()[0]->getSourceNode()->getId(), node_ptr->getId()-1);
			} else {
				// i odd => test layer
				ASSERT_EQ(node_ptr->layer(TEST).getIncomingArcs().size(), 1);
				ASSERT_EQ(node_ptr->getIncomingArcs().size(), 0);
				ASSERT_EQ(node_ptr->layer(TEST).getIncomingArcs()[0]->getSourceNode()->getId(), node_ptr->getId()-1);
			}
		}
	}
}

class Mpi_DistributeMultiGraphWithGhostArcTest : public ::testing::Test {
	protected:
		DistributedGraph<int, GhostData, TestLayer, 2> dg;
		void SetUp() override {
			if(dg.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					dg.buildNode((unsigned long) i, i);
				}
				for (int i = 0; i < dg.getMpiCommunicator().getSize(); i++) {
					if(i % 2 == 0) {
						dg.link(i, (i + 1) % dg.getMpiCommunicator().getSize(), i);
					} else {
						dg.link(i, (i + 1) % dg.getMpiCommunicator().getSize(), i, TestLayer::TEST);
					}
				}
			}
		}

};

TEST_F(Mpi_DistributeMultiGraphWithGhostArcTest, distribute_with_ghost_arc_test) {
	if(dg.getMpiCommunicator().getSize() % 2 != 0) {
		PRINT_PAIR_PROCS_WARNING(distribute_with_arc_test);
	} else {
		dg.distribute();

		ASSERT_EQ(dg.getNodes().size(), 1);
		ASSERT_LT(dg.getNodes().size(), dg.getMpiCommunicator().getSize());

		for(auto node : dg.getNodes()) {
			const Node<SyncDataPtr<int, TestLayer, 2>, TestLayer, 2>* const_node = node.second;
			std::cout << "PROC " << dg.getMpiCommunicator().getRank() << " " << node.first << std::endl;
			if(node.first % 2 == 0) {
				ASSERT_EQ(const_node->getOutgoingArcs().size(), 1);
				ASSERT_EQ(const_node->getIncomingArcs().size(), 0);
				ASSERT_EQ(const_node->layer(TEST).getOutgoingArcs().size(), 0);
				ASSERT_EQ(const_node->layer(TEST).getIncomingArcs().size(), 1);
			} else {
				ASSERT_EQ(const_node->getOutgoingArcs().size(), 0);
				ASSERT_EQ(const_node->getIncomingArcs().size(), 1);
				if(const_node->layer(TEST).getOutgoingArcs().size() != 1)
					std::cout << "PROC ERROR " << dg.getMpiCommunicator().getRank() << " " << node.first << std::endl;
				ASSERT_EQ(const_node->layer(TEST).getOutgoingArcs().size(), 1);
				ASSERT_EQ(const_node->layer(TEST).getIncomingArcs().size(), 0);
			}
		}
	}
}
