#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/graph/distributed_graph.h"

#include "gmock/gmock.h"

using fpmas::communication::MpiCommunicator;
using fpmas::communication::TypedMpi;
using fpmas::graph::DistributedGraph;
using fpmas::graph::DistributedNode;
using fpmas::graph::DistributedEdge;
using fpmas::graph::LocationManager;
using fpmas::synchro::GhostMode;

using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::Ge;

class GhostModeIntegrationTest : public ::testing::Test {
	protected:
		MpiCommunicator comm;
		DistributedGraph<
			unsigned int, GhostMode,
			DistributedNode,
			DistributedEdge,
			TypedMpi,
			LocationManager
			> graph {comm};

		fpmas::api::load_balancing::PartitionMap partition;

		void SetUp() override {
			FPMAS_ON_PROC(comm, 0) {
				std::vector<DistributedId> node_ids;
				for(int i = 0; i < comm.getSize(); i++) {
					auto* node = graph.buildNode(0ul);
					partition[node->getId()] = i;
					node_ids.push_back(node->getId());
				}
				for(std::size_t i = 0; i < node_ids.size() - 1; i ++) {
					graph.link(
						graph.getNode(node_ids[i]),
						graph.getNode(node_ids[i+1]),
						0);
				}
				graph.link(
						graph.getNode(node_ids[node_ids.size()-1]),
						graph.getNode(node_ids[0]),
						0);
			}
			graph.distribute(partition);
		}
};

TEST_F(GhostModeIntegrationTest, remove_node) {
	ASSERT_THAT(graph.getNodes(), SizeIs(Ge(1)));
	if(comm.getSize() > 1) {
		for(auto node : graph.getLocationManager().getLocalNodes()) {
			for(auto neighbor : node.second->outNeighbors())
				graph.removeNode(neighbor);
		}
	} else {
		decltype(graph)::NodeMap nodes = graph.getNodes();
		for(auto node : nodes)
			graph.removeNode(node.second);
	}

	graph.synchronize();

	ASSERT_THAT(graph.getNodes(), IsEmpty());
	ASSERT_THAT(graph.getEdges(), IsEmpty());
}
