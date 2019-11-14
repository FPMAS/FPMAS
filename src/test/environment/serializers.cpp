#include "gtest/gtest.h"
#include "environment/serializers.h"
#include <nlohmann/json.hpp>

using nlohmann::json;
using FPMAS::graph::Graph;
using FPMAS::graph::Node;
using FPMAS::graph::Arc;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g;
	Node<int>* node = g.buildNode("0", new int(0));

	// The node to_json method will be automatically called
	json j = json{{"node", *node}};

	ASSERT_EQ("{\"node\":{\"data\":0,\"id\":\"0\"}}", j.dump());

	delete node->getData();

}

TEST(ArcSerializer, simple_arc_serializer) {
	Graph<int> g;
	Node<int>* n1 = g.buildNode("0", new int(0));
	Node<int>* n2 = g.buildNode("1", new int(1));
	g.link(n1, n2, "0");

	Arc<int>* a = n1->getOutgoingArcs()[0];

	json j = *a;

	ASSERT_EQ(j.dump(), "{\"id\":\"0\",\"link\":[\"0\",\"1\"]}");

	delete n1->getData();
	delete n2->getData();
}

TEST(GraphSerializer, simple_graph_serialization) {

	Graph<int> g;
	Node<int>* n = g.buildNode("0", new int(0));

	json j = g;

	ASSERT_EQ(j.dump(), "{\"arcs\":[],\"nodes\":[{\"data\":0,\"id\":\"0\"}]}");

	delete n->getData();
}

class SampleGraphTest : public ::testing::Test {

	protected:
		Graph<int> simpleGraph;
		std::vector<int*> data = {
			new int(1),
			new int(2),
			new int(3)
		};

		void SetUp() override {
			Node<int>* node1 = simpleGraph.buildNode("1", data[0]);
			Node<int>* node2 = simpleGraph.buildNode("2", data[1]);
			Node<int>* node3 = simpleGraph.buildNode("3", data[2]);

			simpleGraph.link(node1, node2, "0");
			simpleGraph.link(node2, node1, "1");
			simpleGraph.link(node2, node3, "2");

		}

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}

};

TEST_F(SampleGraphTest, sample_graph_serialization) {

	json j = simpleGraph;

	ASSERT_EQ(j.dump(),
			"{\"arcs\":["
				"{\"id\":\"0\",\"link\":[\"1\",\"2\"]},"
				"{\"id\":\"2\",\"link\":[\"2\",\"3\"]},"
				"{\"id\":\"1\",\"link\":[\"2\",\"1\"]}"
				"],"
			"\"nodes\":["
				"{\"data\":3,\"id\":\"3\"},"
				"{\"data\":1,\"id\":\"1\"},"
				"{\"data\":2,\"id\":\"2\"}"
				"]}"
			);

}
