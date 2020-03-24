#ifndef SAMPLE_MULTI_GRAPH_H
#define SAMPLE_MULTI_GRAPH_H

#include "gtest/gtest.h"
#include "utils/test.h"
#include "graph/base/graph.h"

enum TestLayer {
	LAYER_0 = 0,
	LAYER_1 = 1,
	LAYER_2 = 2
};

using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

using FPMAS::test::ASSERT_CONTAINS;

class SampleMultiGraphTest : public ::testing::Test {
	private:
		Graph<int, DefaultId, 3> sampleMultiGraph;
	protected:

		SampleMultiGraphTest() : sampleMultiGraph(0ul) {}

		const Graph<int, DefaultId, 3>& getConstMultiGraph() const {
			return sampleMultiGraph;
		}
		Graph<int, DefaultId, 3>& getMultiGraph() {
			return sampleMultiGraph;
		}

		/*
		 * Layer 0 (default) :
		 * 	- 1 <=> 3 <= 2
		 * Layer 1 :
		 *  - 3 <=> 4
		 * Layer 2 :
		 * 	- 1 => 4 => 2
		 */
		void SetUp() override {
			auto node1 = sampleMultiGraph.buildNode(1);
			auto node2 = sampleMultiGraph.buildNode(2);
			auto node3 = sampleMultiGraph.buildNode(3);
			auto node4 = sampleMultiGraph.buildNode(4);

			// Layer 0 (default)
			sampleMultiGraph.link(node1, node3);
			sampleMultiGraph.link(node3, node1);
			sampleMultiGraph.link(node2, node3);

			// Layer 1
			sampleMultiGraph.link(node3, node4, LAYER_1);
			sampleMultiGraph.link(node4, node3, LAYER_1);

			// Layer 2
			sampleMultiGraph.link(node1, node4, LAYER_2);
			sampleMultiGraph.link(node4, node2, LAYER_2);
		}

		static void testSampleGraphStructure(const Graph<int, DefaultId, 3>& sampleMultiGraph) {
			// NODE 1
			const auto* node1 = sampleMultiGraph.getNode(DefaultId(0ul));
			ASSERT_EQ(node1->layer(LAYER_0).getIncomingArcs(), node1->getIncomingArcs());
			ASSERT_EQ(node1->layer(LAYER_0).getOutgoingArcs(), node1->getOutgoingArcs());

			ASSERT_EQ(node1->getIncomingArcs().size(), 1);
			ASSERT_EQ(node1->getIncomingArcs().at(0)->getSourceNode()->getId(), 2);
			ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->getOutgoingArcs().at(0)->getTargetNode()->getId(), 2);

			ASSERT_EQ(node1->layer(LAYER_1).getIncomingArcs().size(), 0);
			ASSERT_EQ(node1->layer(LAYER_1).getOutgoingArcs().size(), 0);

			ASSERT_EQ(node1->layer(LAYER_2).getIncomingArcs().size(), 0);
			ASSERT_EQ(node1->layer(LAYER_2).getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->layer(LAYER_2).getOutgoingArcs().at(0)->getTargetNode()->getId(), 3);

			// NODE 3
			const auto* node3 = sampleMultiGraph.getNode(DefaultId(2ul));
			ASSERT_EQ(node3->getIncomingArcs().size(), 2);
			ASSERT_CONTAINS(sampleMultiGraph.getArc(0), node3->getIncomingArcs());
			ASSERT_CONTAINS(sampleMultiGraph.getArc(2), node3->getIncomingArcs());

			ASSERT_EQ(node3->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node3->getOutgoingArcs().at(0)->getTargetNode()->getId(), 0);

			ASSERT_EQ(node3->layer(LAYER_1).getIncomingArcs().size(), 1);
			ASSERT_EQ(node3->layer(LAYER_1).getIncomingArcs().at(0)->getSourceNode()->getId(), 3);
			ASSERT_EQ(node3->layer(LAYER_1).getOutgoingArcs().size(), 1);
			ASSERT_EQ(node3->layer(LAYER_1).getOutgoingArcs().at(0)->getTargetNode()->getId(), 3);

			// NODE 4
			const auto* node4 = sampleMultiGraph.getNode(DefaultId(3ul));
			ASSERT_EQ(node4->layer(LAYER_2).getOutgoingArcs().size(), 1);
			ASSERT_EQ(node4->layer(LAYER_2).getOutgoingArcs().at(0)->getTargetNode()->getId(), 1);
			ASSERT_EQ(node4->layer(LAYER_2).getIncomingArcs().size(), 1);
			ASSERT_EQ(node4->layer(LAYER_2).getIncomingArcs().at(0)->getSourceNode()->getId(), 0);

		}

};

#endif
