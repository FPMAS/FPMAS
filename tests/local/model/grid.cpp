#include "fpmas/model/grid.h"
#include "../mocks/model/mock_grid.h"
#include "gtest_environment.h"


using namespace testing;
using fpmas::model::GridCell;

class GridCellTest : public testing::Test {
	protected:
		GridCell grid_cell {{12, -7}};

};

TEST_F(GridCellTest, location) {
	ASSERT_EQ(grid_cell.location(), fpmas::model::DiscretePoint(12, -7));
}


class GridAgentTest : public testing::Test {
	protected:
		model::test::GridAgent grid_agent;
		MockAgentNode mock_agent_node;

		NiceMock<MockGridCell> mock_cell;
		fpmas::graph::DistributedId mock_cell_id {37, 2};
		NiceMock<MockAgentNode> mock_cell_node {mock_cell_id};
		fpmas::model::DiscretePoint location {3, -18};
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
				.WillByDefault(Return(location));
			ON_CALL(mock_cell, node())
				.WillByDefault(Return(&mock_cell_node));
		}
};

TEST_F(GridAgentTest, location) {
	grid_agent.initLocation(&mock_cell);

	ASSERT_EQ(grid_agent.locationPoint(), location);
}

TEST_F(GridAgentTest, json) {
	grid_agent.initLocation(&mock_cell);
	nlohmann::json j = fpmas::api::utils::PtrWrapper<model::test::GridAgent::JsonBase>(&grid_agent);

	auto unserialized_agent = j.get<fpmas::api::utils::PtrWrapper<model::test::GridAgent::JsonBase>>();
	ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<model::test::GridAgent*>(NotNull()));
	ASSERT_EQ(static_cast<model::test::GridAgent*>(unserialized_agent.get())->locationId(), mock_cell_id);
	ASSERT_EQ(static_cast<model::test::GridAgent*>(unserialized_agent.get())->locationPoint(), location);

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
	ASSERT_EQ(static_cast<model::test::GridAgentWithData*>(unserialized_agent.get())->locationPoint(), location);
	ASSERT_EQ(static_cast<model::test::GridAgentWithData*>(unserialized_agent.get())->data, 8);

	delete unserialized_agent.get();

	/* Tear Down */
	delete model::test::GridAgentWithData::mobility_range;
	delete model::test::GridAgentWithData::perception_range;
}
