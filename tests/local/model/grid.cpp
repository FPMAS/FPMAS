#include "fpmas/model/grid.h"
#include "../mocks/model/mock_grid.h"
#include "gtest_environment.h"


using namespace testing;
using fpmas::model::GridCell;
using fpmas::model::EnvironmentLayers;

class GridCellTest : public testing::Test {
	protected:
		GridCell grid_cell {{12, -7}};

};

TEST_F(GridCellTest, location) {
	ASSERT_EQ(grid_cell.location(), fpmas::model::DiscretePoint(12, -7));
}


class GridAgentTest : public testing::Test, protected model::test::GridAgent {
	protected:
		model::test::GridAgent& grid_agent {*this};
		MockAgentNode mock_agent_node;

		NiceMock<MockGridCell> mock_cell;
		fpmas::graph::DistributedId mock_cell_id {37, 2};
		NiceMock<MockAgentNode> mock_cell_node {mock_cell_id, &mock_cell};
		fpmas::model::DiscretePoint location_point {3, -18};
		NiceMock<MockModel> mock_model;

		static void SetUpTestSuite() {
			model::test::GridAgent::mobility_range
				= new NiceMock<MockRange<fpmas::api::model::GridCell>>;
			model::test::GridAgent::perception_range
				= new NiceMock<MockRange<fpmas::api::model::GridCell>>;
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
		= new NiceMock<MockRange<fpmas::api::model::GridCell>>;
	model::test::GridAgentWithData::perception_range
		= new NiceMock<MockRange<fpmas::api::model::GridCell>>;

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
	std::vector<fpmas::model::AgentNode*> move_neighbors {&mock_cell_node};
	MockAgentEdge mock_cell_edge;
	mock_cell_edge.setSourceNode(&mock_agent_node);
	mock_cell_edge.setTargetNode(&mock_cell_node);
	std::vector<fpmas::model::AgentEdge*> move_neighbor_edges {&mock_cell_edge};

	ON_CALL(mock_agent_node, outNeighbors(EnvironmentLayers::MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(mock_agent_node, getOutgoingEdges(EnvironmentLayers::MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	EXPECT_CALL(mock_model, link(this, &mock_cell, EnvironmentLayers::NEW_LOCATION));
	EXPECT_CALL(mock_model, unlink(&mock_cell_edge));

	this->moveTo(location_point);
}

TEST_F(GridAgentTest, moveToPointOutOfField) {
	ASSERT_THROW(this->moveTo(location_point), fpmas::api::model::OutOfMobilityFieldException);
}
