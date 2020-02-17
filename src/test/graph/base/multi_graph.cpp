#include "sample_multi_graph.h"

TEST_F(SampleMultiGraphTest, buildTest) {
	testSampleGraphStructure(getMultiGraph());
}

TEST_F(SampleMultiGraphTest, removeNodeTest) {
	auto fossil = getMultiGraph().removeNode(3ul);
	ASSERT_EQ(fossil.incomingArcs.size(), 0);
	ASSERT_EQ(fossil.outgoingArcs.size(), 0);
	ASSERT_EQ(getMultiGraph().getArcs().size(), 2);

	// Node 1
	// Incoming
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_0).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_1).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_2).getIncomingArcs().size(), 0);

	// Outgoing
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_0).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_1).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(1)->layer(LAYER_2).getOutgoingArcs().size(), 1);

	// Node 2
	// Incoming
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_0).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_1).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_2).getIncomingArcs().size(), 1);

	// Outgoing
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_0).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_1).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(2)->layer(LAYER_2).getOutgoingArcs().size(), 0);

	// Node 4
	// Incoming
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_0).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_1).getIncomingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_2).getIncomingArcs().size(), 1);

	// Outgoing
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_0).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_1).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_2).getOutgoingArcs().size(), 1);

}

TEST_F(SampleMultiGraphTest, unlinkTest) {
	getMultiGraph().unlink(3ul);

	ASSERT_EQ(getConstMultiGraph().getArcs().size(), 6);
	// Layer 0
	ASSERT_EQ(getConstMultiGraph().getNode(3)->layer(LAYER_0).getOutgoingArcs().size(), 1);
	ASSERT_EQ(getConstMultiGraph().getNode(3)->layer(LAYER_0).getIncomingArcs().size(), 2);
	// Layer 1
	ASSERT_EQ(getConstMultiGraph().getNode(3)->layer(LAYER_1).getOutgoingArcs().size(), 0);
	ASSERT_EQ(getConstMultiGraph().getNode(4)->layer(LAYER_1).getIncomingArcs().size(), 0);
}
