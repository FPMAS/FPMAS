#include "fpmas/graph/analysis.h"
#include "fpmas/api/graph/location_state.h"
#include "fpmas/graph/clustered_graph_builder.h"
#include "fpmas/graph/graph_builder.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/utils/macros.h"

#include "gmock/gmock.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace testing;

class DistantNodesOutgoingNeighborsTest : public Test {
	protected:
		const fpmas::api::graph::LayerId LAYER = 2;
		fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph {
				fpmas::communication::WORLD
				};

		void SetUp() override {
			fpmas::random::PoissonDistribution<std::size_t> edge_dist(6);
			fpmas::graph::DistributedClusteredGraphBuilder<int> graph_builder(
					edge_dist);

			fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
					10 * graph.getMpiCommunicator().getSize(),
					graph.getMpiCommunicator()
					);
			graph_builder.build(node_builder, LAYER, graph);
		}

		void check_map_entries(
				std::unordered_map<DistributedId, std::vector<DistributedId>>& outnodes) {
			auto graph_nodes = graph.getNodes();
			// There is the proper count of distinct nodes in the map...
			ASSERT_THAT(outnodes, SizeIs(graph.getLocationManager().getDistantNodes().size()));
			for(auto item : outnodes) {
				ASSERT_THAT(graph_nodes.find(item.first), Not(Eq(graph_nodes.end())));
				// ... and they are all DISTANT nodes
				ASSERT_THAT(graph_nodes.at(item.first)->state(), Eq(fpmas::api::graph::DISTANT));
				// So the map necessarily contains an entry for each DISTANT node.
			}

		}
};

TEST_F(DistantNodesOutgoingNeighborsTest, test) {

	auto outnodes = fpmas::graph::distant_nodes_outgoing_neighbors(graph, LAYER);
	check_map_entries(outnodes);

	// Send back results to all processes to perform a check
	fpmas::communication::TypedMpi<std::vector<std::pair<DistributedId, std::vector<DistributedId>>>> mpi(graph.getMpiCommunicator());
	std::unordered_map<
		int,
		std::vector<std::pair<DistributedId, std::vector<DistributedId>>>
			> data;
	for(auto item : outnodes) {
		data[graph.getNode(item.first)->location()]
			.push_back({item.first, item.second});
	}
	data = mpi.allToAll(data);

	for(auto item : data) {
		for(auto node : item.second) {
			std::set<DistributedId> expected;
			for(auto edge : graph.getNode(node.first)->getOutgoingEdges(LAYER))
				expected.insert(edge->getTargetNode()->getId());
			std::set<DistributedId> received;
			for(auto neighbor : node.second)
				received.insert(neighbor);

			ASSERT_THAT(received, ElementsAreArray(expected));
		}
	}
}

TEST_F(DistantNodesOutgoingNeighborsTest, test_empty_layer) {
	// Nothing on layer 7
	auto outnodes = fpmas::graph::distant_nodes_outgoing_neighbors(graph, 7);
	// There must still be an entry for each distant node
	check_map_entries(outnodes);

	for(auto item : outnodes)
		ASSERT_THAT(item.second, SizeIs(0));
}

TEST(ClusteringCoefficient, no_edge) {
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph {
		fpmas::communication::WORLD
	};
	// Builds 2 nodes on each process, do not link them to anything
	graph.buildNode(0);
	graph.buildNode(0);

	// Evaluates the clustering coefficient on a random layer
	double c = fpmas::graph::clustering_coefficient(graph, 7);
	ASSERT_EQ(c, 0);
}

/*
 * checks that the clustering coefficient of a complete graph is 1.
 */
TEST(ClusteringCoefficient, complete_graph) {
	const fpmas::graph::LayerId LAYER = 3;
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph {
		fpmas::communication::WORLD
	};
	fpmas::api::graph::PartitionMap partition;
	FPMAS_ON_PROC(graph.getMpiCommunicator(), 0) {
		for(int i = 0; i < 2*graph.getMpiCommunicator().getSize(); i++) {
			auto node = graph.buildNode(0);
			partition[node->getId()] = i/2;
		}
		for(auto source : graph.getNodes()) {
			for(auto target : graph.getNodes()) {
				if(source != target)
					graph.link(source.second, target.second, LAYER);
			}
		}
	}
	graph.distribute(partition);

	double c = fpmas::graph::clustering_coefficient(graph, LAYER);
	ASSERT_EQ(c, 1);

	// 0 on empty layers
	c = fpmas::graph::clustering_coefficient(graph, 0);
	ASSERT_EQ(c, 0);
}
