#ifndef SAMPLE_GRAPH_H
#define SAMPLE_GRAPH_H

#include "gtest/gtest.h"
#include "graph/base/graph.h"
#include "utils/test.h"

using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

using FPMAS::test::ASSERT_CONTAINS;

class SampleGraphTest : public ::testing::Test {

	protected:
		Graph<int, unsigned long> sampleGraph;

		void SetUp() override {
			auto node1 = sampleGraph.buildNode(1ul, 1);
			auto node2 = sampleGraph.buildNode(2ul, 2);
			auto node3 = sampleGraph.buildNode(3ul, 3);

			sampleGraph.link(node1, node2, 0ul);
			sampleGraph.link(node2, node1, 1ul);
			sampleGraph.link(node2, node3, 2ul);

		}

		static void testSampleGraphStructure(Graph<int, unsigned long>* sampleGraph) {
			auto node1 = sampleGraph->getNode(1ul);
			ASSERT_EQ(node1->data(), 1);
			ASSERT_EQ(node1->getIncomingArcs().size(), 1);
			ASSERT_EQ(node1->getIncomingArcs().at(0), sampleGraph->getArc(1ul));
			ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->getOutgoingArcs().at(0), sampleGraph->getArc(0ul));

			auto node2 = sampleGraph->getNode(2ul);
			ASSERT_EQ(node2->data(), 2);
			ASSERT_EQ(node2->getIncomingArcs().size(), 1);
			ASSERT_EQ(node2->getIncomingArcs().at(0), sampleGraph->getArc(0ul));
			ASSERT_EQ(node2->getOutgoingArcs().size(), 2);

			ASSERT_CONTAINS(sampleGraph->getArc(1ul), node2->getOutgoingArcs());
			ASSERT_CONTAINS(sampleGraph->getArc(2ul), node2->getOutgoingArcs());

			auto node3 = sampleGraph->getNode(3ul);
			ASSERT_EQ(node3->data(), 3);
			ASSERT_EQ(node3->getIncomingArcs().size(), 1);
			ASSERT_EQ(node3->getIncomingArcs().at(0), sampleGraph->getArc(2ul));
			ASSERT_EQ(node3->getOutgoingArcs().size(), 0);
		}

};
#endif
