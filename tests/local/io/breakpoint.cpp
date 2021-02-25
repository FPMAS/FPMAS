#include "fpmas/io/breakpoint.h"
#include "../mocks/synchro/mock_sync_mode.h"
#include "../mocks/communication/mock_communication.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/random/random.h"

using namespace testing;

struct Data {
	int i;
	std::string message;
};

bool operator==(const Data& d1, const Data& d2) {
	return d1.i == d2.i && d1.message == d2.message;
}

static void to_json(nlohmann::json& j, const Data& data) {
	j["i"] = data.i;
	j["m"] = data.message;
}

static void from_json(const nlohmann::json& j, Data& data) {
	data.i = j.at("i").get<int>();
	data.message = j.at("m").get<std::string>();
}

TEST(Breakpoint, simple) {
	std::stringstream str;

	fpmas::io::Breakpoint<Data> breakpoint;
	Data out;
	out.i = 12;
	out.message = "hello";

	breakpoint.dump(str, out);

	Data in;
	breakpoint.load(str, in);

	ASSERT_EQ(out, in);
}

template<typename T>
using NiceMockSyncMode = NiceMock<MockSyncMode<T>>;


class NodeMatcher {
	private:
		fpmas::api::graph::DistributedNode<int>* node_to_match;
		static bool edge_eq(
				std::vector<fpmas::api::graph::DistributedEdge<int>*> edges_to_match,
				std::vector<fpmas::api::graph::DistributedEdge<int>*> edges) {
			std::vector<fpmas::graph::DistributedId> ids_to_match;
			std::vector<fpmas::graph::DistributedId> ids;
			for(auto edge : edges_to_match)
				ids_to_match.push_back(edge->getId());
			for(auto edge : edges)
				ids.push_back(edge->getId());
			return Matches(UnorderedElementsAreArray(ids_to_match))(ids);
		}

	public:
		NodeMatcher(
				fpmas::api::graph::DistributedNode<int>* node_to_match)
			: node_to_match(node_to_match) {}

		bool operator()(fpmas::api::graph::DistributedNode<int>* n) const {
			return 
				node_to_match->getId() == n->getId() &&
				node_to_match->state() == n->state() &&
				node_to_match->location() == n->location() &&
				Matches(FloatEq(node_to_match->getWeight()))(n->getWeight()) &&
				node_to_match->data() == n->data() &&
				edge_eq(node_to_match->getIncomingEdges(), n->getIncomingEdges()) &&
				edge_eq(node_to_match->getOutgoingEdges(), n->getOutgoingEdges());
		}
};

class EdgeMatcher {
	private:
		fpmas::api::graph::DistributedEdge<int>* edge_to_match;

	public:
		EdgeMatcher(fpmas::api::graph::DistributedEdge<int>* edge_to_match)
			: edge_to_match(edge_to_match) {}

		bool operator()(fpmas::api::graph::DistributedEdge<int>* e) const {
			return
				edge_to_match->getId() == e->getId() &&
				edge_to_match->getLayer() == e->getLayer() &&
				edge_to_match->state() == e->state() &&
				Matches(FloatEq(edge_to_match->getWeight()))(e->getWeight()) &&
				edge_to_match->getSourceNode()->getId() == edge_to_match->getSourceNode()->getId() &&
				edge_to_match->getTargetNode()->getId() == edge_to_match->getTargetNode()->getId();
		}
};

TEST(Breakpoint, distributed_graph) {
	MockMpiCommunicator<3, 8> comm;

	NiceMock<MockSyncLinker<int>> mock_sync_linker;
	fpmas::graph::DistributedGraph<int, NiceMockSyncMode> graph(comm);
	ON_CALL(const_cast<MockSyncMode<int>&>(static_cast<const MockSyncMode<int>&>(graph.getSyncMode())), getSyncLinker())
		.WillByDefault(ReturnRef(mock_sync_linker));

	std::vector<fpmas::api::graph::DistributedNode<int>*> nodes;
	for(std::size_t i = 0; i < 10; i++) {
		nodes.push_back(graph.buildNode(i));
		// This process
		nodes.back()->setLocation(3);
	}
	
	fpmas::random::mt19937_64 rd;
	// Some random ranks, different from the current process
	fpmas::random::UniformIntDistribution<int> rd_rank(4, 7);
	for(std::size_t i = 0; i < 5; i++) {
		nodes.push_back(
				graph.insertDistant(new fpmas::graph::DistributedNode<int>({2, (unsigned int) i}, i))
				);
		nodes.back()->setLocation(rd_rank(rd));
	}

	fpmas::random::UniformIntDistribution<std::size_t> rd_id(0, nodes.size()-1);
	fpmas::random::UniformIntDistribution<fpmas::graph::LayerId> rd_layer(0, 5);

	for(std::size_t i = 0; i < 20; i++)
		graph.link(nodes[rd_id(rd)], nodes[rd_id(rd)], rd_layer(rd));

	std::stringstream stream;

	nlohmann::json j;
	fpmas::io::Breakpoint<fpmas::api::graph::DistributedGraph<int>> breakpoint;
	breakpoint.dump(stream, graph);

	fpmas::graph::DistributedGraph<int, NiceMockSyncMode> breakpoint_graph(comm);
	breakpoint.load(stream, breakpoint_graph);

	/*
	 * Check Graph state
	 */
	std::vector<Matcher<std::pair<fpmas::graph::DistributedId, fpmas::api::graph::DistributedNode<int>*>>> node_matchers;
	for(auto node : graph.getNodes())
		node_matchers.push_back(Pair(node.first, Truly(NodeMatcher(node.second))));

	ASSERT_THAT(breakpoint_graph.getNodes(), UnorderedElementsAreArray(node_matchers));

	std::vector<Matcher<std::pair<fpmas::graph::DistributedId, fpmas::api::graph::DistributedEdge<int>*>>> edge_matchers;
	for(auto edge : graph.getEdges())
		edge_matchers.push_back(Pair(edge.first, Truly(EdgeMatcher(edge.second))));

	ASSERT_THAT(breakpoint_graph.getEdges(), UnorderedElementsAreArray(edge_matchers));

	/*
	 * Check LocationManager state
	 */
	node_matchers.clear();
	ASSERT_THAT(
			breakpoint_graph.getLocationManager().getCurrentLocations(),
			UnorderedElementsAreArray(graph.getLocationManager().getCurrentLocations())
			);
	for(auto node : graph.getLocationManager().getLocalNodes())
		// Mathches only node pointers, to ensure pointers in LocationManager
		// correspond to nodes in the breakpoint_graph
		node_matchers.push_back(Pair(node.first, Eq(breakpoint_graph.getNode(node.first))));
	ASSERT_THAT(
			breakpoint_graph.getLocationManager().getLocalNodes(),
			UnorderedElementsAreArray(node_matchers)
			);
	node_matchers.clear();
	for(auto node : graph.getLocationManager().getDistantNodes())
		// Mathches only node pointers, to ensure pointers in LocationManager
		// correspond to nodes in the breakpoint_graph
		node_matchers.push_back(Pair(node.first, Eq(breakpoint_graph.getNode(node.first))));
	ASSERT_THAT(
			breakpoint_graph.getLocationManager().getDistantNodes(),
			UnorderedElementsAreArray(node_matchers)
			);
}
