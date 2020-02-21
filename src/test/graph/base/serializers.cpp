#include "gtest/gtest.h"
#include "graph/base/serializers.h"
#include <nlohmann/json.hpp>

#include "sample_graph.h"

using nlohmann::json;
using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g;
	auto node = g.buildNode(85250, 3.2, 0);

	// The node to_json method will be automatically called
	json j = json{{"node", *node}};

	// Note : the float conversion is actually perfectly normal
	// See https://float.exposed/0x404ccccd
	ASSERT_EQ("{\"node\":{\"data\":0,\"id\":85250,\"weight\":3.200000047683716}}", j.dump());

}

TEST(NodeSerializer, node_serialization_without_data_or_weight) {

}

TEST(NodeSerializer, simple_node_deserialization) {

	json node_json = R"(
		{"data":0,"id":85250,"weight":2.3}
		)"_json;

	Node<int, DefaultLayer, 1> node = node_json.get<Node<int, DefaultLayer, 1>>();

	ASSERT_EQ(node.data(), 0);
	ASSERT_EQ(node.getId(), 85250ul);
	ASSERT_EQ(node.getWeight(), 2.3f);
}

TEST(NodeSerializer, node_deserialization_no_data) {
	// Unserialization without data
	json node_json = R"(
		{"id":85250}
		)"_json;

	Node<int, DefaultLayer, 1> node = node_json.get<Node<int, DefaultLayer, 1>>();

	ASSERT_EQ(node.getId(), 85250ul);

	node.data() = 2;
	ASSERT_EQ(node.data(), 2);
}

enum TestLayer {
	_Default = 0,
	Test = 1
};

TEST(ArcSerializer, simple_arc_serializer) {
	Graph<int, TestLayer, 2> g;
	Node<int, TestLayer, 2>* n1 = g.buildNode(0ul, 0);
	Node<int, TestLayer, 2>* n2 = g.buildNode(1ul, 1);
	g.link(n1, n2, 0ul, TestLayer::Test);

	const Node<int, TestLayer, 2>* n = g.getNode(0ul);
	Arc<int, TestLayer, 2>* a = n->layer(TestLayer::Test).getOutgoingArcs()[0];

	json j = *a;

	ASSERT_EQ(j.dump(), "{\"id\":0,\"layer\":1,\"link\":[0,1]}");
}

TEST(ArcSerializer, simple_arc_deserializer) {
	json j = R"({
		"id":2,
		"layer":1,
		"link":[0,1]
		})"_json;

	Arc<int, TestLayer, 2> arc = j;
	ASSERT_EQ(arc.getId(), 2);
	ASSERT_EQ(arc.layer, TestLayer::Test);
	ASSERT_EQ(arc.getSourceNode()->getId(), 0);
	ASSERT_EQ(arc.getTargetNode()->getId(), 1);

	delete arc.getSourceNode();
	delete arc.getTargetNode();
}

TEST(GraphSerializer, simple_graph_serialization) {

	Graph<int> g;
	Node<int, DefaultLayer, 1>* n = g.buildNode(0ul, 0);

	json j = g;

	ASSERT_EQ(j.dump(), "{\"arcs\":[],\"nodes\":[{\"data\":0,\"id\":0,\"weight\":1.0}]}");
}

TEST_F(SampleGraphTest, sample_graph_serialization) {

	json j = sampleGraph;

	ASSERT_EQ(j.dump(),
			"{\"arcs\":["
				"{\"id\":2,\"layer\":0,\"link\":[2,3]},"
				"{\"id\":0,\"layer\":0,\"link\":[1,2]},"
				"{\"id\":1,\"layer\":0,\"link\":[2,1]}"
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
	ASSERT_EQ(g.getNodes().find(0ul)->second->data(), 0);
}

TEST_F(SampleGraphTest, sample_graph_deserialization) {

	json graph_json = R"(
			{
			"arcs":[
				{"id":0,"layer":0,"link":[1,2]},
				{"id":2,"layer":0,"link":[2,3]},
				{"id":1,"layer":0,"link":[2,1]}
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
}
