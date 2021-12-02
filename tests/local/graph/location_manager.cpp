#include "fpmas/graph/location_manager.h"

#include "communication/mock_communication.h"
#include "graph/mock_distributed_node.h"
#include "synchro/mock_mutex.h"

using namespace testing;

using fpmas::graph::LocationManager;

class LocationManagerTest : public Test {
	protected:
		typedef typename
			LocationManager<int>::NodeMap
			NodeMap;
		MockMpiCommunicator<2, 5> comm;
		MockMpi<DistributedId> id_mpi {comm};
		MockMpi<std::pair<DistributedId, int>> location_mpi {comm};

		LocationManager<int>
			location_manager {comm, id_mpi, location_mpi};
};

TEST_F(LocationManagerTest, setLocal) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(5, 4)};
	EXPECT_CALL(mock_node, setState(LocationState::LOCAL));
	EXPECT_CALL(mock_node, setLocation(2));

	location_manager.setLocal(&mock_node);

	ASSERT_THAT(
		location_manager.getLocalNodes(),
		ElementsAre(Key(DistributedId(5, 4)))
	);
}

TEST_F(LocationManagerTest, setDistant) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(5, 4)};
	EXPECT_CALL(mock_node, setState(LocationState::DISTANT));
	location_manager.setDistant(&mock_node);

	ASSERT_THAT(
		location_manager.getDistantNodes(),
		ElementsAre(Key(DistributedId(5, 4)))
	);
}

TEST_F(LocationManagerTest, removeLocal) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(5, 4)};
	EXPECT_CALL(mock_node, setState);
	location_manager.setLocal(&mock_node);

	location_manager.remove(&mock_node);
	ASSERT_THAT(location_manager.getLocalNodes(), IsEmpty());
}

TEST_F(LocationManagerTest, removeDistant) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(5, 4)};
	EXPECT_CALL(mock_node, setState);
	location_manager.setDistant(&mock_node);

	location_manager.remove(&mock_node);
	ASSERT_THAT(location_manager.getDistantNodes(), IsEmpty());
}

TEST_F(LocationManagerTest, addManagedNode) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(2, 0)};
	location_manager.addManagedNode(mock_node.getId(), 2);
	ASSERT_THAT(location_manager.getCurrentLocations(), ElementsAre(
				Pair(DistributedId(2, 0), 2)
				));
}

TEST_F(LocationManagerTest, removeManagedNode) {
	MockDistributedNode<int, NiceMock> mock_node {DistributedId(2, 0)};
	location_manager.addManagedNode(mock_node.getId(), 2);

	location_manager.removeManagedNode(mock_node.getId());
	ASSERT_THAT(location_manager.getCurrentLocations(), IsEmpty());
}

class LocationManagerUpdateTest : public LocationManagerTest {
	protected:
		typedef
		std::unordered_map<int, std::vector<DistributedId>>
		node_id_map;

		typedef
		std::unordered_map<int, std::pair<DistributedId, int>>
		location_map;

		MockDistributedNode<int, NiceMock>* nodes[3] = {
			new MockDistributedNode<int, NiceMock>(DistributedId(2, 0)),
			new MockDistributedNode<int, NiceMock>(DistributedId(2, 1)),
			new MockDistributedNode<int, NiceMock>(DistributedId(2, 2))
		};


		std::unordered_map<DistributedId, MockDistributedNode<int, NiceMock>*> mock_local {
			// Node [2, 2], that was previously on proc 7, is now local
			{DistributedId(2, 2), new MockDistributedNode<int, NiceMock>(DistributedId(2, 2))},
			// Other arbitrary node
			{DistributedId(1, 4), new MockDistributedNode<int, NiceMock>(DistributedId(1, 4))}
		};

		NodeMap local;
		
		std::unordered_map<DistributedId, MockDistributedNode<int, NiceMock>*> distant {
			// Distant node with this proc has origin
			{DistributedId(2, 1), new MockDistributedNode<int, NiceMock>(DistributedId(2, 1))},
			// Other arbitrary node
			{DistributedId(4, 0), new MockDistributedNode<int, NiceMock>(DistributedId(4, 0))}
		};

		void SetUp() override {
			for(auto item : mock_local) {
				local[item.first] = item.second;
			}
			location_manager.addManagedNode(nodes[0]->getId(), 1);
			location_manager.addManagedNode(nodes[1]->getId(), 4);
			location_manager.addManagedNode(nodes[2]->getId(), 7);
			delete nodes[0];
			delete nodes[1];
			delete nodes[2];

			// Location of local nodes should be updated to be this proc
			EXPECT_CALL(*mock_local.at(DistributedId(2, 2)), setLocation(2));
			EXPECT_CALL(*mock_local.at(DistributedId(1, 4)), setLocation(2));

			// Export [1, 4] location (this proc) to its origin proc (proc 1)
			auto export_update_matcher = ElementsAre(
				Pair(1, ElementsAre(DistributedId(1, 4)))
				);

			// Received updates of node managed by this proc ([2, 0], [2, 1]),
			// except for those located on this proc ([2, 2])
			std::unordered_map<int, std::vector<DistributedId>> import_map {
				{1, {DistributedId(2, 0)}}, // [2, 0] is still on 1
				{5, {DistributedId(2, 1)}} // [2, 1] is now on 5
			};

			// Location of distant node [2, 1] should be updated to 5 in the
			// first step, since its origin is this proc
			EXPECT_CALL(*distant.at(DistributedId(2, 1)), setLocation(5));

			// Mock comm
			EXPECT_CALL(id_mpi, migrate(export_update_matcher)
				).WillOnce(Return(import_map));

			// Request for location of distant nodes ([4, 0]) to their origin
			// proc, except for those that have this proc as origin ([2, 1])
			// since their location should have been updated in the previous
			// step.
			auto export_requests_matcher = ElementsAre(
				Pair(4, ElementsAre(DistributedId(4, 0)))
				);

			std::unordered_map<int, std::vector<DistributedId>> importRequests {
				// For some reason, proc 0 wants to know the current location
				// of [2, 0] and [2, 2].
				{0, {DistributedId(2, 1), DistributedId(2, 2)}},
				// Proc 7 also wants the current location of [2, 0]
				{7, {DistributedId(2, 0)}}
			};

			// Mock comm
			EXPECT_CALL(id_mpi, migrate(export_requests_matcher))
				.WillOnce(Return(importRequests));

			auto export_responses_matcher = UnorderedElementsAre(
					// Previously updated locations sent to 0
					Pair(0, UnorderedElementsAre(
							std::pair<DistributedId, int> {DistributedId(2, 1), 5},
							std::pair<DistributedId, int> {DistributedId(2, 2), 2}
							)),
					// Previously updated locations sent to 7
					Pair(7, ElementsAre(
							std::pair<DistributedId, int> {DistributedId(2, 0), 1}
							))
					);
			// Received location for [4, 0] from 4 : proc 9
			std::unordered_map<int, std::vector<std::pair<DistributedId, int>>> imported_responses {
				{4, {{DistributedId(4, 0), 9}}}
			};
			
			// Location of distant node [4, 0] should be updated to 9
			EXPECT_CALL(*distant.at(DistributedId(4, 0)), setLocation(9));

			// Mock comm
			EXPECT_CALL(location_mpi, migrate(export_responses_matcher))
				.WillOnce(Return(imported_responses));
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

TEST_F(LocationManagerUpdateTest, updateLocations) {
	for(auto node : distant) {
		location_manager.setDistant(node.second);
	}
	for(auto node : local) {
		location_manager.setLocal(node.second);
	}
	location_manager.updateLocations();

	auto locationMap = location_manager.getCurrentLocations();
	ASSERT_EQ(locationMap.at(DistributedId(2, 0)), 1);
	ASSERT_EQ(locationMap.at(DistributedId(2, 1)), 5);
	ASSERT_EQ(locationMap.at(DistributedId(2, 2)), 2);
}
