#include "gtest/gtest.h"

#include <typeinfo>

#include "graph/parallel/olz.h"
#include "communication/communication.h"
#include "utils/config.h"
#include "graph/parallel/distributed_graph.h"

using FPMAS::communication::MpiCommunicator;

using FPMAS::graph::parallel::DistributedGraph;

class Mpi_OlzTest : public ::testing::Test {
	protected:
		DistributedGraph<int> dg = DistributedGraph<int>();

		DistributedId id1;
		DistributedId id2;
		DistributedId id3;

		void SetUp() override {
			id1 = dg.buildNode()->getId();
			id2 = dg.buildNode()->getId();
			id3 = dg.buildNode(2)->getId();

			dg.link(id1, id3);
			dg.link(id3, id2);
		}
};

TEST_F(Mpi_OlzTest, simpleGhostNodeTest) {
	// Builds ghost node from node 2
	auto node = dg.getNode(id3);	
	dg.getGhost().buildNode(*node);

	// Node 0 is linked to the ghost node
	ASSERT_EQ(dg.getNode(id1)->getOutgoingArcs().size(), 2);
	// Node 1 is linked to the ghost node
	ASSERT_EQ(dg.getNode(id2)->getIncomingArcs().size(), 2);
	// Nothing has changed for node 2
	ASSERT_EQ(dg.getNode(id3)->getIncomingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(id3)->getOutgoingArcs().size(), 1);

	// Removes the real node
	dg.removeNode(id3);

	// Only the ghost node persist
	ASSERT_EQ(dg.getNodes().size(), 2);
	ASSERT_EQ(dg.getNode(id1)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(id1)->getIncomingArcs().size(), 0);
	ASSERT_EQ(dg.getNode(id2)->getIncomingArcs().size(), 1);
	ASSERT_EQ(dg.getNode(id2)->getOutgoingArcs().size(), 0);

	// Ghost node data is accessible from node 0
	ASSERT_EQ(
			dg.getNode(id1)->getOutgoingArcs().at(0)->getTargetNode()->data()->read(),
			2
			);
	ASSERT_EQ(
			dg.getNode(id1)->getOutgoingArcs().at(0)->getTargetNode()->getId(),
			id3
			);

	// Ghost node data is accessible from node 1
	ASSERT_EQ(
			dg.getNode(id2)->getIncomingArcs().at(0)->getSourceNode()->data()->read(),
			2
			);
	ASSERT_EQ(
			dg.getNode(id2)->getIncomingArcs().at(0)->getSourceNode()->getId(),
			id3
			);
	dg.synchronize();

}
