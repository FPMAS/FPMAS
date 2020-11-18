#include "fpmas/synchro/ghost/ghost_mode.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/graph/mock_location_manager.h"
#include "../mocks/synchro/mock_mutex.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;
using fpmas::synchro::NodeUpdatePack;

using fpmas::synchro::ghost::GhostDataSync;

namespace fpmas { namespace synchro {
	template<typename T>
		bool operator==(const NodeUpdatePack<T>& d1, const NodeUpdatePack<T>& d2) {
			return d1.id == d2.id && d1.updated_data == d2.updated_data && d1.updated_weight == d2.updated_weight;
		}
}}

class GhostDataSyncTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int> NodeType;
		typedef MockDistributedEdge<int> EdgeType;
		typedef typename MockDistributedGraph<int, NodeType, EdgeType>::NodeMap NodeMap;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mock_comm;
		MockMpi<fpmas::synchro::NodeUpdatePack<int>> data_mpi {mock_comm};
		MockMpi<DistributedId> id_mpi {mock_comm};
		MockMpi<std::pair<DistributedId, int>> location_mpi {mock_comm};
		MockDistributedGraph<int, NodeType, EdgeType> mocked_graph;

		GhostDataSync<int>
			dataSync {data_mpi, id_mpi, mocked_graph};

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
			ON_CALL(mocked_graph, getMpiCommunicator())
				.WillByDefault(ReturnRef(mock_comm));
			EXPECT_CALL(mocked_graph, getMpiCommunicator()).Times(AnyNumber());
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
		Pair(0, UnorderedElementsAre(
				NodeUpdatePack<int>(DistributedId(2, 0), nodes[0]->data(), 2.8f),
				NodeUpdatePack<int>(DistributedId(7, 1), nodes[3]->data(), 2.3f)
				)),
		Pair(1, ElementsAre(NodeUpdatePack<int>(DistributedId(2, 0), nodes[0]->data(), 2.8f))),
		Pair(5, ElementsAre(NodeUpdatePack<int>(DistributedId(6, 2), nodes[2]->data(), 0.4f)))
		);
	EXPECT_CALL(data_mpi, migrate(export_node_matcher));

	dataSync.synchronize();
}

TEST_F(GhostDataSyncTest, import_test) {
	NodeMap graph_nodes {
		{DistributedId(2, 0), nodes[0]},
		{DistributedId(0, 0), nodes[1]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	EXPECT_CALL(*nodes[1], location)
		.WillRepeatedly(Return(0));
	EXPECT_CALL(*nodes[2], location)
		.WillRepeatedly(Return(0));
	EXPECT_CALL(*nodes[3], location)
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

	std::unordered_map<int, std::vector<NodeUpdatePack<int>>> updated_data {
		{0, {{DistributedId(0, 0), 12, 14.6f}, {DistributedId(6, 2), 56, 7.2f}}},
		{9, {{DistributedId(7, 1), 125, 2.2f}}}
	};
	EXPECT_CALL(data_mpi, migrate(IsEmpty()))
		.WillOnce(Return(updated_data));

	EXPECT_CALL(*nodes[1], setWeight(14.6));
	EXPECT_CALL(*nodes[2], setWeight(7.2));
	EXPECT_CALL(*nodes[3], setWeight(2.2));

	dataSync.synchronize();

	ASSERT_EQ(nodes[1]->data(), 12);
	ASSERT_EQ(nodes[2]->data(), 56);
	ASSERT_EQ(nodes[3]->data(), 125);
}
