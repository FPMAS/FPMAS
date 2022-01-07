#include "fpmas/model/spatial/grid.h"
#include "fpmas/model/spatial/von_neumann.h"
#include "fpmas/model/spatial/moore.h"
#include "../../../mocks/synchro/mock_mutex.h"
#include "../../../mocks/model/mock_grid.h"
#include "gtest_environment.h"

#include <unordered_set>

using namespace testing;
using namespace fpmas::model;

class GridCellTest : public testing::Test {
	protected:
		GridCell grid_cell {{12, -7}};
		fpmas::api::model::AgentPtr src_ptr {&grid_cell};

		void TearDown() {
			src_ptr.release();
		}
};

TEST_F(GridCellTest, location) {
	ASSERT_EQ(grid_cell.location(), DiscretePoint(12, -7));
}

TEST_F(GridCellTest, json) {
	nlohmann::json j = src_ptr;

	auto ptr = j.get<fpmas::api::model::AgentPtr>();
	ASSERT_THAT(ptr.get(), WhenDynamicCastTo<GridCell*>(NotNull()));
	ASSERT_EQ(
			dynamic_cast<GridCell*>(ptr.get())->location(),
			DiscretePoint(12, -7));
}


class GridAgentTest : public testing::Test, protected model::test::GridAgent {
	protected:
		model::test::GridAgent& grid_agent {*this};
		MockAgentNode<NiceMock> mock_agent_node {{0, 0}};

		MockGridCell<NiceMock> mock_cell;
		fpmas::graph::DistributedId mock_cell_id {37, 2};
		MockAgentNode<NiceMock> mock_cell_node {mock_cell_id, &mock_cell};
		NiceMock<MockMutex<AgentPtr>> mock_cell_mutex;
		DiscretePoint location_point {3, -18};
		NiceMock<MockModel> mock_model;

		static void SetUpTestSuite() {
			model::test::GridAgent::mobility_range
				= new NiceMock<MockRange<MockGridCell<NiceMock>>>;
			model::test::GridAgent::perception_range
				= new NiceMock<MockRange<MockGridCell<NiceMock>>>;
		}
		static void TearDownTestSuite() {
			delete model::test::GridAgent::mobility_range;
			delete model::test::GridAgent::perception_range;
		}

		void SetUp() {
			grid_agent.setModel(&mock_model);
			grid_agent.setNode(&mock_agent_node);

			ON_CALL(mock_cell, location)
				.WillByDefault(Return(location_point));
			ON_CALL(mock_cell, node())
				.WillByDefault(Return(&mock_cell_node));
			ON_CALL(Const(mock_cell), node())
				.WillByDefault(Return(&mock_cell_node));
			ON_CALL(mock_cell_node, mutex())
				.WillByDefault(Return(&mock_cell_mutex));
			ON_CALL(Const(mock_cell_node), mutex())
				.WillByDefault(Return(&mock_cell_mutex));
			ON_CALL(mock_cell_mutex, read)
				.WillByDefault(ReturnRef(mock_cell_node.data()));
		}

		void TearDown() {
			mock_cell_node.data().release();
		}
};

TEST_F(GridAgentTest, location) {
	grid_agent.initLocation(&mock_cell);

	ASSERT_EQ(grid_agent.locationPoint(), location_point);
}

TEST_F(GridAgentTest, json) {
	grid_agent.initLocation(&mock_cell);
	nlohmann::json j = fpmas::api::utils::PtrWrapper<model::test::GridAgent::JsonBase>(&grid_agent);

	auto unserialized_agent = j.get<fpmas::api::utils::PtrWrapper<model::test::GridAgent::JsonBase>>();
	ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<model::test::GridAgent*>(NotNull()));
	ASSERT_EQ(static_cast<model::test::GridAgent*>(unserialized_agent.get())->locationId(), mock_cell_id);
	ASSERT_EQ(static_cast<model::test::GridAgent*>(unserialized_agent.get())->locationPoint(), location_point);

	delete unserialized_agent.get();
}

TEST_F(GridAgentTest, to_json_with_data) {
	// GridAgentWithData is only used in this test, so set up and tear down is
	// performed directly in this test

	/* Set Up */
	model::test::GridAgentWithData::mobility_range
		= new NiceMock<MockRange<MockGridCell<NiceMock>>>;
	model::test::GridAgentWithData::perception_range
		= new NiceMock<MockRange<MockGridCell<NiceMock>>>;

	model::test::GridAgentWithData agent(8);
	agent.setModel(&mock_model);
	agent.setNode(&mock_agent_node);
	agent.initLocation(&mock_cell);

	/* to_json */
	nlohmann::json j = fpmas::api::utils::PtrWrapper<model::test::GridAgentWithData::JsonBase>(&agent);

	/* from_json */
	auto unserialized_agent = j.get<fpmas::api::utils::PtrWrapper<model::test::GridAgentWithData::JsonBase>>();

	/* check */
	ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<model::test::GridAgentWithData*>(NotNull()));
	ASSERT_EQ(static_cast<model::test::GridAgentWithData*>(unserialized_agent.get())->locationId(), mock_cell_id);
	ASSERT_EQ(static_cast<model::test::GridAgentWithData*>(unserialized_agent.get())->locationPoint(), location_point);
	ASSERT_EQ(static_cast<model::test::GridAgentWithData*>(unserialized_agent.get())->data, 8);

	delete unserialized_agent.get();

	/* Tear Down */
	delete model::test::GridAgentWithData::mobility_range;
	delete model::test::GridAgentWithData::perception_range;
}

TEST_F(GridAgentTest, moveToPoint) {
	// Build MOVE layer
	std::vector<AgentNode*> move_neighbors {&mock_cell_node};
	MockAgentEdge<NiceMock> mock_cell_edge;
	mock_cell_edge.setSourceNode(&mock_agent_node);
	mock_cell_edge.setTargetNode(&mock_cell_node);
	std::vector<AgentEdge*> move_neighbor_edges {&mock_cell_edge};

	ON_CALL(mock_agent_node, outNeighbors(SpatialModelLayers::MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(mock_agent_node, getOutgoingEdges(SpatialModelLayers::MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	EXPECT_CALL(mock_model, link(this, &mock_cell, SpatialModelLayers::NEW_LOCATION));
	EXPECT_CALL(mock_model, unlink(&mock_cell_edge));

	this->moveTo(location_point);
}

TEST_F(GridAgentTest, moveToPointOutOfField) {
	ASSERT_THROW(this->moveTo(location_point), fpmas::api::model::OutOfMobilityFieldException);
}

TEST(VonNeumannRange, radius_von_neumann_grid) {
	VonNeumannRange<VonNeumannGrid<>> range(5);

	ASSERT_EQ(range.radius(nullptr), 5);
}


TEST(VonNeumannRange, range) {
	MockGridCell<NiceMock>* current_location = new MockGridCell<NiceMock>;
	ON_CALL(*current_location, location)
		.WillByDefault(Return(DiscretePoint(3, 4)));

	std::vector<MockGridCell<NiceMock>*> cells;
	std::vector<DiscretePoint> grid_points;
	for(DiscreteCoordinate x = 0; x <= 6; x++) {
		for(DiscreteCoordinate y = 1; y <= 7; y++) {
			auto cell = new MockGridCell<NiceMock>;
			ON_CALL(*cell, location)
				.WillByDefault(Return(DiscretePoint(x, y)));
			cells.push_back(cell);
		}
	}
	std::unordered_set<DiscretePoint> points_in_range {
						{3, 2},
				{2, 3}, {3, 3}, {4, 3},
		{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4},
				{2, 5}, {3, 5}, {4, 5},
						{3, 6}
	};
	
	VonNeumannRange<VonNeumannGrid<MockGridCell<NiceMock>>> range(2);

	for(auto cell : cells)
		ASSERT_EQ(range.contains(current_location, cell), points_in_range.count(cell->location())>0);

	for(auto cell : cells)
		delete cell;
	delete current_location;
}

TEST(MooreRange, radius_von_neumann_grid) {
	MooreRange<VonNeumannGrid<>> range(5);

	ASSERT_EQ(range.radius(nullptr), 10);
}


TEST(MooreRange, range) {
	MockGridCell<NiceMock>* current_location = new MockGridCell<NiceMock>;
	ON_CALL(*current_location, location)
		.WillByDefault(Return(DiscretePoint(3, 4)));

	std::vector<MockGridCell<NiceMock>*> cells;
	std::vector<DiscretePoint> grid_points;
	for(DiscreteCoordinate x = 0; x <= 6; x++) {
		for(DiscreteCoordinate y = 1; y <= 7; y++) {
			auto cell = new MockGridCell<NiceMock>;
			ON_CALL(*cell, location)
				.WillByDefault(Return(DiscretePoint(x, y)));
			cells.push_back(cell);
		}
	}
	std::unordered_set<DiscretePoint> points_in_range;
	for(DiscreteCoordinate x = 1; x <= 5; x++)
		for(DiscreteCoordinate y = 2; y <= 6; y++)
			points_in_range.insert({x, y});
	
	MooreRange<VonNeumannGrid<MockGridCell<NiceMock>>> range(2);

	for(auto cell : cells)
		ASSERT_EQ(range.contains(current_location, cell), points_in_range.count(cell->location())>0);

	for(auto cell : cells)
		delete cell;
	delete current_location;
}
