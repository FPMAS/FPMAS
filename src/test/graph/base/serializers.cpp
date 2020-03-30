#include "gtest/gtest.h"
#include "graph/base/serializers.h"
#include <nlohmann/json.hpp>

#include "sample_graph.h"

using nlohmann::json;
using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g(0ul);
	auto node = g.buildNode(3.2, 0);

	// The node to_json method will be automatically called
	json j = json{{"node", *node}};

	// Note : the float conversion is actually perfectly normal
	// See https://float.exposed/0x404ccccd
	ASSERT_EQ("{\"node\":{\"data\":0,\"id\":0,\"weight\":3.200000047683716}}", j.dump());

}

TEST(NodeSerializer, node_serialization_without_data_or_weight) {

}

TEST(NodeSerializer, simple_node_deserialization) {

	json node_json = R"(
		{"data":0,"id":85250,"weight":2.3}
		)"_json;

	Node<int, DefaultId> node = node_json.get<Node<int, DefaultId>>();

	ASSERT_EQ(node.data(), 0);
	ASSERT_EQ(node.getId(), DefaultId(85250ul));
	ASSERT_EQ(node.getWeight(), 2.3f);
}

enum TestLayer {
	_Default = 0,
	Test = 1
};

TEST(ArcSerializer, simple_arc_serializer) {
	Graph<int, DefaultId> g(0ul);
	auto* n1 = g.buildNode(0);
	auto* n2 = g.buildNode(1);
	auto* a = g.link(n1, n2, TestLayer::Test);

	json j = *a;

	ASSERT_EQ(j.dump(), "{\"id\":0,\"layer\":1,\"link\":[0,1]}");
}

TEST(ArcSerializer, simple_arc_deserializer) {
	json j = R"({
		"id":2,
		"layer":1,
		"link":[0,1]
		})"_json;

	Arc<int, DefaultId> arc = j;
	ASSERT_EQ(arc.getId(), DefaultId(2));
	ASSERT_EQ(arc.layer, TestLayer::Test);
	ASSERT_EQ(arc.getSourceNode()->getId(), DefaultId(0));
	ASSERT_EQ(arc.getTargetNode()->getId(), DefaultId(1));

	delete arc.getSourceNode();
	delete arc.getTargetNode();
}

TEST(GraphSerializer, simple_graph_serialization) {

	Graph<int> g(0ul);
	g.buildNode(0);

	json j = g;

	ASSERT_EQ(j.dump(), "{\"arcs\":[],\"nodes\":[{\"data\":0,\"id\":0,\"weight\":1.0}]}");
}

TEST_F(SampleGraphTest, sample_graph_serialization) {

	json j = sampleGraph;

	ASSERT_EQ(j.dump(),
			"{\"arcs\":["
				"{\"id\":2,\"layer\":0,\"link\":[1,2]},"
				"{\"id\":0,\"layer\":0,\"link\":[0,1]},"
				"{\"id\":1,\"layer\":0,\"link\":[1,0]}"
				"],"
			"\"nodes\":["
				"{\"data\":3,\"id\":2,\"weight\":1.0},"
				"{\"data\":1,\"id\":0,\"weight\":1.0},"
				"{\"data\":2,\"id\":1,\"weight\":1.0}"
				"]}"
			);

}
