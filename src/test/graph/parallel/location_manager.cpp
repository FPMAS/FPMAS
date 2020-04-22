#include "graph/parallel/location_manager.h"

#include "api/communication/mock_communication.h"
#include "api/graph/parallel/mock_distributed_node.h"
#include "api/graph/parallel/synchro/mock_mutex.h"

using ::testing::UnorderedElementsAre;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::AnyOf;

using FPMAS::graph::parallel::LocationManager;

class LocationManagerTest : public ::testing::Test {
	protected:
		typedef MockDistributedNode<int, MockMutex> node_type;
		
		typedef typename
			LocationManager<MockDistributedNode<int, MockMutex>>::node_map
			node_map;
		typedef
		std::unordered_map<int, std::vector<DistributedId>>
		node_id_map;

		typedef
		std::unordered_map<int, std::pair<DistributedId, int>>
		location_map;

		MockMpiCommunicator<2, 5> comm;
		node_type* nodes[3] = {
			new node_type(DistributedId(2, 0)),
			new node_type(DistributedId(2, 1)),
			new node_type(DistributedId(2, 2))
		};

		LocationManager<MockDistributedNode<int, MockMutex>>
			locationManager {comm};


		node_map local {
			// Node [2, 2], that was previously on proc 7, is now local
			{DistributedId(2, 2), new node_type(DistributedId(2, 2))},
			// Other arbitrary node
			{DistributedId(1, 4), new node_type(DistributedId(1, 4))}
		};
		
		node_map distant {
			// Distant node with this proc has origin
			{DistributedId(2, 1), new node_type(DistributedId(2, 1))},
			// Other arbitrary node
			{DistributedId(4, 0), new node_type(DistributedId(4, 0))}
		};

		void SetUp() override {
			locationManager.addManagedNode(nodes[0], 1);
			locationManager.addManagedNode(nodes[1], 4);
			locationManager.addManagedNode(nodes[2], 7);
			delete nodes[0];
			delete nodes[1];
			delete nodes[2];

			// Location of local nodes should be updated to be this proc
			EXPECT_CALL(*local.at(DistributedId(2, 2)), setLocation(2));
			EXPECT_CALL(*local.at(DistributedId(1, 4)), setLocation(2));

			// Export [1, 4] location (this proc) to its origin proc (proc 1)
			auto exportUpdateMatcher = ElementsAre(
				Pair(1, nlohmann::json({DistributedId(1, 4)}).dump())
				);

			// Received updates of node managed by this proc ([2, 0], [2, 1]),
			// except for those located on this proc ([2, 2])
			std::unordered_map<int, std::string> importMap {
				{1, nlohmann::json({DistributedId(2, 0)}).dump()}, // [2, 0] is still on 1
				{5, nlohmann::json({DistributedId(2, 1)}).dump()} // [2, 1] is now on 5
			};

			// Location of distant node [2, 1] should be updated to 5 in the
			// first step, since its origin is this proc
			EXPECT_CALL(*distant.at(DistributedId(2, 1)), setLocation(5));

			// Mock comm
			EXPECT_CALL(comm, allToAll(exportUpdateMatcher))
				.WillOnce(Return(importMap));

			// Request for location of distant nodes ([4, 0]) to their origin
			// proc, except for those that have this proc as origin ([2, 1])
			// since their location should have been updated in the previous
			// step.
			auto exportRequestsMatcher = ElementsAre(
				Pair(4, nlohmann::json({DistributedId(4, 0)}).dump())
				);

			std::unordered_map<int, std::string> importRequests {
				// For some reason, proc 0 wants to know the current location
				// of [2, 0] and [2, 2].
				{0, nlohmann::json({DistributedId(2, 1), DistributedId(2, 2)}).dump()},
				// Proc 7 also wants the current location of [2, 0]
				{7, nlohmann::json({DistributedId(2, 0)}).dump()}
			};

			// Mock comm
			EXPECT_CALL(comm, allToAll(exportRequestsMatcher))
				.WillOnce(Return(importRequests));

			auto exportResponsesMatcher = UnorderedElementsAre(
					// Previously updated locations sent to 0
					Pair(0, AnyOf(
							nlohmann::json({
							{DistributedId(2, 1), 5},
							{DistributedId(2, 2), 2}
							}).dump(),
							nlohmann::json({
							{DistributedId(2, 2), 2},
							{DistributedId(2, 1), 5}
							}).dump())),
					// Previously updated locations sent to 7
					Pair(7, nlohmann::json({
							{DistributedId(2, 0), 1}
							}).dump())
					);
			// Received location for [4, 0] from 4 : proc 9
			std::unordered_map<int, std::string> importedResponses {
				{4, nlohmann::json({{DistributedId(4, 0), 9}}).dump()}
			};
			
			// Location of distant node [4, 0] should be updated to 9
			EXPECT_CALL(*distant.at(DistributedId(4, 0)), setLocation(9));

			// Mock comm
			EXPECT_CALL(comm, allToAll(exportResponsesMatcher))
				.WillOnce(Return(importedResponses));
		}

		void TearDown() override {
			for(auto node : local) {
				delete node.second;
			}
			for(auto node : distant) {
				delete node.second;
			}
		}
};

TEST_F(LocationManagerTest, updateLocations) {
	locationManager.updateLocations(local, distant);

	auto locationMap = locationManager.getCurrentLocations();
	ASSERT_EQ(locationMap.at(DistributedId(2, 0)), 1);
	ASSERT_EQ(locationMap.at(DistributedId(2, 1)), 5);
	ASSERT_EQ(locationMap.at(DistributedId(2, 2)), 2);
}
