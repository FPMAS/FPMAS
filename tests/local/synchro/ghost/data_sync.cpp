#include "fpmas/synchro/ghost/ghost_mode.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/synchro/mock_mutex.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

using fpmas::synchro::ghost::GhostDataSync;

class GhostDataSyncTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int> NodeType;
		typedef MockDistributedArc<int> ArcType;
		typedef typename MockDistributedGraph<int, NodeType, ArcType>::NodeMap NodeMap;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mock_comm;
		MockMpi<fpmas::graph::parallel::NodePtrWrapper<int>> node_mpi {mock_comm};
		MockMpi<DistributedId> id_mpi {mock_comm};
		MockMpi<std::pair<DistributedId, int>> location_mpi {mock_comm};
		MockDistributedGraph<int, NodeType, ArcType> mocked_graph;

		GhostDataSync<int>
			dataSync {node_mpi, id_mpi, mocked_graph};

		MockLocationManager<int> location_manager {mock_comm, id_mpi, location_mpi};


		std::array<NodeType*, 4> nodes {
			new NodeType(DistributedId(2, 0), 1, 2.8),
			new NodeType(DistributedId(0, 0), 2, 0.1),
			new NodeType(DistributedId(6, 2), 10, 0.4),
			new NodeType(DistributedId(7, 1), 5, 2.3),
		};

		void SetUp() override {
			ON_CALL(mocked_graph, getLocationManager)
				.WillByDefault(ReturnRef(location_manager));
			EXPECT_CALL(mocked_graph, getLocationManager).Times(AnyNumber());
		}

		void TearDown() override {
			for(auto node : nodes)
				delete node;

		}

		void setUpGraphNodes(NodeMap& graph_nodes) {
			ON_CALL(mocked_graph, getNodes)
				.WillByDefault(ReturnRef(graph_nodes));
			EXPECT_CALL(mocked_graph, getNodes).Times(AnyNumber());

			for(auto node : graph_nodes) {
				EXPECT_CALL(mocked_graph, getNode(node.first))
					.WillRepeatedly(Return(node.second));
			}
		}
};

TEST_F(GhostDataSyncTest, export_data) {
	NodeMap graph_nodes {
		{DistributedId(2, 0), nodes[0]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	setUpGraphNodes(graph_nodes);

	NodeMap distant_nodes;
	EXPECT_CALL(location_manager, getDistantNodes)
		.WillRepeatedly(ReturnRef(distant_nodes));

	std::unordered_map<int, std::vector<DistributedId>> requests {
		{0, {DistributedId(2, 0), DistributedId(7, 1)}},
		{1, {DistributedId(2, 0)}},
		{5, {DistributedId(6, 2)}}
	};
	EXPECT_CALL(id_mpi, migrate(IsEmpty()))
		.WillOnce(Return(requests));

	auto export_node_matcher = UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(nodes[0], nodes[3])),
		Pair(1, ElementsAre(nodes[0])),
		Pair(5, ElementsAre(nodes[2]))
		);
	EXPECT_CALL(node_mpi, migrate(export_node_matcher));

	dataSync.synchronize();
}

TEST_F(GhostDataSyncTest, import_test) {
	NodeMap graph_nodes {
		{DistributedId(2, 0), nodes[0]},
		{DistributedId(0, 0), nodes[1]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	EXPECT_CALL(*nodes[1], getLocation)
		.WillRepeatedly(Return(0));
	EXPECT_CALL(*nodes[2], getLocation)
		.WillRepeatedly(Return(0));
	EXPECT_CALL(*nodes[3], getLocation)
		.WillRepeatedly(Return(9));

	NodeMap distant_nodes {
		{DistributedId(0, 0), nodes[1]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	EXPECT_CALL(location_manager, getDistantNodes)
		.WillRepeatedly(ReturnRef(distant_nodes));

	setUpGraphNodes(graph_nodes);
	auto requests_matcher = UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(DistributedId(0, 0), DistributedId(6, 2))),
		Pair(9, ElementsAre(DistributedId(7, 1)))
	);
	EXPECT_CALL(id_mpi, migrate(requests_matcher));

	std::unordered_map<int, std::vector<fpmas::graph::parallel::NodePtrWrapper<int>>> updated_data {
		{0, {new NodeType {DistributedId(0, 0), 12, 14.6}, new NodeType {DistributedId(6, 2), 56, 7.2}}},
		{9, {new NodeType {DistributedId(7, 1), 125, 2.2}}}
	};
	EXPECT_CALL(node_mpi, migrate(IsEmpty()))
		.WillOnce(Return(updated_data));

	EXPECT_CALL(*nodes[1], setWeight(14.6));
	EXPECT_CALL(*nodes[2], setWeight(7.2));
	EXPECT_CALL(*nodes[3], setWeight(2.2));

	dataSync.synchronize();

	ASSERT_EQ(nodes[1]->data(), 12);
	ASSERT_EQ(nodes[2]->data(), 56);
	ASSERT_EQ(nodes[3]->data(), 125);
}
