#include "fpmas/model/grid.h"
#include "../mocks/model/mock_grid.h"
#include "gtest_environment.h"


using namespace testing;
using fpmas::model::GridCell;
using model::test::GridAgent;
using model::test::GridAgentWithData;

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
		}
};

TEST_F(GridAgentTest, location) {
	grid_agent.moveToCell(&mock_cell);

	ASSERT_EQ(grid_agent.currentLocation(), location);
}

TEST_F(GridAgentTest, json) {
	grid_agent.moveToCell(&mock_cell);
	nlohmann::json j = fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<GridAgent>>(&grid_agent);

	auto unserialized_agent = j.get<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<GridAgent>>>();
	ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<GridAgent*>(NotNull()));
	ASSERT_EQ(unserialized_agent->currentLocation(), location);

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

	GridAgentWithData agent(8);
	agent.setModel(&mock_model);
	agent.setNode(&mock_agent_node);
	agent.moveToCell(&mock_cell);

	/* to_json */
	nlohmann::json j = fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<GridAgentWithData>>(&agent);

	/* from_json */
	auto unserialized_agent = j.get<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<GridAgentWithData>>>();

	/* check */
	ASSERT_EQ(unserialized_agent->currentLocation(), location);
	ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<GridAgentWithData*>(NotNull()));
	ASSERT_EQ(dynamic_cast<GridAgentWithData*>(unserialized_agent.get())->data, 8);

	delete unserialized_agent.get();

	/* Tear Down */
	delete model::test::GridAgentWithData::mobility_range;
	delete model::test::GridAgentWithData::perception_range;
}
