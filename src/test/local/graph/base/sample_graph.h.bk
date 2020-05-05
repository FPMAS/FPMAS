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
		Graph<int> sampleGraph;

		SampleGraphTest() : sampleGraph(0ul) {}

		void SetUp() override {
			auto node1 = sampleGraph.buildNode(1);
			auto node2 = sampleGraph.buildNode(2);
			auto node3 = sampleGraph.buildNode(3);

			sampleGraph.link(node1, node2);
			sampleGraph.link(node2, node1);
			sampleGraph.link(node2, node3);

		}

		static void testSampleGraphStructure(Graph<int>* sampleGraph) {
			auto node1 = sampleGraph->getNode(DefaultId(0ul));
			ASSERT_EQ(node1->getId(), DefaultId(0ul));
			ASSERT_EQ(node1->data(), 1);
			ASSERT_EQ(node1->getIncomingArcs().size(), 1);
			ASSERT_EQ(node1->getIncomingArcs().at(0), sampleGraph->getArc(DefaultId(1ul)));
			ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->getOutgoingArcs().at(0), sampleGraph->getArc(DefaultId(0ul)));

			auto node2 = sampleGraph->getNode(DefaultId(1ul));
			ASSERT_EQ(node2->getId(), DefaultId(1ul));
			ASSERT_EQ(node2->data(), 2);
			ASSERT_EQ(node2->getIncomingArcs().size(), 1);
			ASSERT_EQ(node2->getIncomingArcs().at(0), sampleGraph->getArc(DefaultId(0ul)));
			ASSERT_EQ(node2->getOutgoingArcs().size(), 2);

			ASSERT_CONTAINS(sampleGraph->getArc(DefaultId(1ul)), node2->getOutgoingArcs());
			ASSERT_CONTAINS(sampleGraph->getArc(DefaultId(2ul)), node2->getOutgoingArcs());

			auto node3 = sampleGraph->getNode(DefaultId(2ul));
			ASSERT_EQ(node3->getId(), DefaultId(2ul));
			ASSERT_EQ(node3->data(), 3);
			ASSERT_EQ(node3->getIncomingArcs().size(), 1);
			ASSERT_EQ(node3->getIncomingArcs().at(0), sampleGraph->getArc(DefaultId(2ul)));
			ASSERT_EQ(node3->getOutgoingArcs().size(), 0);
		}

};
#endif
