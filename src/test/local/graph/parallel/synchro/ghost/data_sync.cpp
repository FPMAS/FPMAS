#include "graph/parallel/synchro/ghost/basic_ghost_mode.h"

#include "../mocks/communication/mock_communication.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_location_manager.h"
#include "../mocks/graph/parallel/synchro/mock_mutex.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

using FPMAS::graph::parallel::synchro::ghost::GhostDataSync;

class GhostDataSyncTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int, MockMutex> node_type;
		typedef MockDistributedArc<int, MockMutex> arc_type;
		typedef typename MockDistributedGraph<node_type, arc_type>::node_map node_map;

		static const int current_rank = 3;
		MockMpiCommunicator<current_rank, 10> mockComm;
		GhostDataSync<node_type, arc_type, MockMpi>
			dataSync {mockComm};

		MockLocationManager<node_type> locationManager {mockComm};

		MockDistributedGraph<node_type, arc_type> mockedGraph;

		std::array<node_type*, 4> nodes {
			new node_type(DistributedId(2, 0), 1, 2.8),
			new node_type(DistributedId(0, 0), 2, 0.1),
			new node_type(DistributedId(6, 2), 10, 0.4),
			new node_type(DistributedId(7, 1), 5, 2.3),
		};

		void SetUp() override {
			ON_CALL(mockedGraph, getLocationManager)
				.WillByDefault(ReturnRef(locationManager));
			EXPECT_CALL(mockedGraph, getLocationManager).Times(AnyNumber());
		}

		void TearDown() override {
			for(auto node : nodes)
				delete node;

		}

		void setUpGraphNodes(node_map& graphNodes) {
			ON_CALL(mockedGraph, getNodes)
				.WillByDefault(ReturnRef(graphNodes));
			EXPECT_CALL(mockedGraph, getNodes).Times(AnyNumber());

			for(auto node : graphNodes) {
				EXPECT_CALL(mockedGraph, getNode(node.first))
					.WillRepeatedly(Return(node.second));
			}
		}
};

//class Foo {
	//private:
		//int data;

	//public:
		//Foo() {}
		//Foo(const Foo& other) {
			//std::cout << "copy construct" << std::endl;
		//}
		//Foo& operator=(const Foo& other) = delete;
		/*
		 *Foo& operator=(const Foo& other) {
		 *    std::cout << "copy assign" << std::endl;
		 *    return *this;
		 *}
		 */
//};

//TEST(TEST_FOO, foo) {
	//Foo i;
	//Foo& j = i;
	//{
		//Foo k;
		//j = k;
	//}
//}

//TEST(TEST_REF, ref_test) {
	//int i = 10;
	//int& j = i;
	//{
		//int k = 8;
		//j = k;
	//}
	//std::cout << j << std::endl;
/*}*/

TEST_F(GhostDataSyncTest, export_data) {
	node_map graphNodes {
		{DistributedId(2, 0), nodes[0]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	setUpGraphNodes(graphNodes);

	node_map distantNodes;
	EXPECT_CALL(locationManager, getDistantNodes)
		.WillRepeatedly(ReturnRef(distantNodes));

	std::unordered_map<int, std::vector<DistributedId>> requests {
		{0, {DistributedId(2, 0), DistributedId(7, 1)}},
		{1, {DistributedId(2, 0)}},
		{5, {DistributedId(6, 2)}}
	};
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(dataSync.getDistIdMpi()), migrate(IsEmpty()))
		.WillOnce(Return(requests));

	auto exportDataMatcher = UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(*nodes[0], *nodes[3])),
		Pair(1, ElementsAre(*nodes[0])),
		Pair(5, ElementsAre(*nodes[2]))
		);
	EXPECT_CALL(const_cast<MockMpi<node_type>&>(dataSync.getNodeMpi()), migrate(exportDataMatcher));

	dataSync.synchronize(mockedGraph);
}

TEST_F(GhostDataSyncTest, import_test) {
	node_map graphNodes {
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

	node_map distantNodes {
		{DistributedId(0, 0), nodes[1]},
		{DistributedId(6, 2), nodes[2]},
		{DistributedId(7, 1), nodes[3]}
	};
	EXPECT_CALL(locationManager, getDistantNodes)
		.WillRepeatedly(ReturnRef(distantNodes));

	setUpGraphNodes(graphNodes);
	auto requestsMatcher = UnorderedElementsAre(
		Pair(0, UnorderedElementsAre(DistributedId(0, 0), DistributedId(6, 2))),
		Pair(9, ElementsAre(DistributedId(7, 1)))
	);
	EXPECT_CALL(const_cast<MockMpi<DistributedId>&>(dataSync.getDistIdMpi()), migrate(requestsMatcher));

	std::unordered_map<int, std::vector<node_type>> updatedData {
		{0, {{DistributedId(0, 0), 12, 14.6}, {DistributedId(6, 2), 56, 7.2}}},
		{9, {{DistributedId(7, 1), 125, 2.2}}}
	};
	EXPECT_CALL(const_cast<MockMpi<node_type>&>(dataSync.getNodeMpi()), migrate(IsEmpty()))
		.WillOnce(Return(updatedData));

	EXPECT_CALL(*nodes[1], setWeight(14.6));
	EXPECT_CALL(*nodes[2], setWeight(7.2));
	EXPECT_CALL(*nodes[3], setWeight(2.2));

	dataSync.synchronize(mockedGraph);

	ASSERT_EQ(nodes[1]->data(), 12);
	ASSERT_EQ(nodes[2]->data(), 56);
	ASSERT_EQ(nodes[3]->data(), 125);
}
