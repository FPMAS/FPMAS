#include "gtest/gtest.h"
#include "environment/graph/base/serializers.h"
#include <nlohmann/json.hpp>

#include "sample_graph.h"

using nlohmann::json;
using FPMAS::graph::Graph;
using FPMAS::graph::Node;
using FPMAS::graph::Arc;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g;
	Node<int>* node = g.buildNode(85250, 3.2, 0);

	// The node to_json method will be automatically called
	json j = json{{"node", *node}};

	// Note : the float conversion is actually perfectly normal
	// See https://float.exposed/0x404ccccd
	ASSERT_EQ("{\"node\":{\"data\":0,\"id\":85250,\"weight\":3.200000047683716}}", j.dump());

	// delete node->getData();

}

TEST(NodeSerializer, node_serialization_without_data_or_weight) {

}

TEST(NodeSerializer, simple_node_deserialization) {

	json node_json = R"(
		{"data":0,"id":85250,"weight":2.3}
		)"_json;

	Node<int> node = node_json.get<Node<int>>();

	ASSERT_EQ(node.getData(), 0);
	ASSERT_EQ(node.getId(), 85250ul);
	ASSERT_EQ(node.getWeight(), 2.3f);

	// In this case, data has been allocated at unserialization time and should
	// be deleted manually
	// delete node.getData();
}

TEST(NodeSerializer, node_deserialization_no_data) {
	// Unserialization without data
	json node_json = R"(
		{"id":85250}
		)"_json;

	Node<int> node = node_json.get<Node<int>>();

	ASSERT_EQ(node.getId(), 85250ul);

	node.setData(2);
	ASSERT_EQ(node.getData(), 2);

}

TEST(ArcSerializer, simple_arc_serializer) {
	Graph<int> g;
	Node<int>* n1 = g.buildNode(0ul, 0);
	Node<int>* n2 = g.buildNode(1ul, 1);
	g.link(n1, n2, 0ul);

	Arc<int>* a = n1->getOutgoingArcs()[0];

	json j = *a;

	ASSERT_EQ(j.dump(), "{\"id\":0,\"link\":[0,1]}");

	// delete n1->getData();
	// delete n2->getData();
}

TEST(ArcSerializer, simple_arc_deserializer) {
	json j = R"({
		"id":2,
		"link":[0,1]
		})"_json;

	Arc<int> arc = j;
	ASSERT_EQ(arc.getSourceNode()->getId(), 0);
	ASSERT_EQ(arc.getTargetNode()->getId(), 1);

	delete arc.getSourceNode();
	delete arc.getTargetNode();
}

TEST(GraphSerializer, simple_graph_serialization) {

	Graph<int> g;
	Node<int>* n = g.buildNode(0ul, 0);

	json j = g;

	ASSERT_EQ(j.dump(), "{\"arcs\":[],\"nodes\":[{\"data\":0,\"id\":0,\"weight\":1.0}]}");

	// delete n->getData();
}

TEST_F(SampleGraphTest, sample_graph_serialization) {

	json j = sampleGraph;

	ASSERT_EQ(j.dump(),
			"{\"arcs\":["
				"{\"id\":2,\"link\":[2,3]},"
				"{\"id\":0,\"link\":[1,2]},"
				"{\"id\":1,\"link\":[2,1]}"
				"],"
			"\"nodes\":["
				"{\"data\":3,\"id\":3,\"weight\":1.0},"
				"{\"data\":1,\"id\":1,\"weight\":1.0},"
				"{\"data\":2,\"id\":2,\"weight\":1.0}"
				"]}"
			);

}

TEST(GraphSerializer, simple_graph_deserialization) {
	json graph_json = R"(
		{
			"arcs":[],
			"nodes":[{"data":0,"id":0}]
		}
		)"_json;

	Graph<int> g = graph_json.get<Graph<int>>();
	ASSERT_EQ(g.getNodes().size(), 1);
	ASSERT_EQ(g.getNodes().count(0ul), 1);
	ASSERT_EQ(g.getNodes().find(0ul)->second->getId(), 0ul);
	ASSERT_EQ(g.getNodes().find(0ul)->second->getData(), 0);

	// delete g.getNodes().find(0ul)->second->getData();
}

TEST_F(SampleGraphTest, sample_graph_deserialization) {

	json graph_json = R"(
			{
			"arcs":[
				{"id":0,"link":[1,2]},
				{"id":2,"link":[2,3]},
				{"id":1,"link":[2,1]}
				],
			"nodes":[
				{"data":3,"id":3},
				{"data":1,"id":1},
				{"data":2,"id":2}
				]
			}
			)"_json;

	Graph<int> graph = graph_json.get<Graph<int>>();

	testSampleGraphStructure(&graph);

	for(auto node : graph.getNodes()) {
		// delete node.second->getData();
	}

}
