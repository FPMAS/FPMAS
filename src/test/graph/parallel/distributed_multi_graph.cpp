#include "gtest/gtest.h"

#include "graph/parallel/distributed_graph.h"

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

	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_LT(dg.getArcs().size(), dg.getMpiCommunicator().getSize());

	std::cout << "hey there" << std::endl;

	for(auto node : dg.getNodes()) {
		const Node<SyncDataPtr<int, TestLayer, 2>, TestLayer, 2>* node_ptr = node.second;
		if(node.first % 2 == 0) {
			ASSERT_EQ(dg.getNodes().count(node.first + 1), 1);
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
			ASSERT_EQ(dg.getNodes().count(node.first - 1), 1);
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
