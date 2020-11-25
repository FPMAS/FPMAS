#include "fpmas/model/grid.h"
#include "../mocks/model/mock_grid.h"
#include "../mocks/random/mock_random.h"
#include "gtest_environment.h"


using namespace testing;
using fpmas::model::GridCell;
using fpmas::model::SpatialModelLayers;
using fpmas::model::VonNeumannGridBuilder;

class GridCellTest : public testing::Test {
	protected:
		GridCell grid_cell {{12, -7}};
		fpmas::api::model::AgentPtr src_ptr {&grid_cell};

		void TearDown() {
			src_ptr.release();
		}
};

TEST_F(GridCellTest, location) {
	ASSERT_EQ(grid_cell.location(), fpmas::model::DiscretePoint(12, -7));
}

TEST_F(GridCellTest, json) {
	nlohmann::json j = src_ptr;

	auto ptr = j.get<fpmas::api::model::AgentPtr>();
	ASSERT_THAT(ptr.get(), WhenDynamicCastTo<GridCell*>(NotNull()));
	ASSERT_EQ(
			dynamic_cast<GridCell*>(ptr.get())->location(),
			fpmas::model::DiscretePoint(12, -7));
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

TEST(VonNeumannGridBuilder, grid_cell_factory) {
	VonNeumannGridBuilder default_builder (10, 10);
	ASSERT_THAT(
			default_builder.gridCellFactory(),
			Ref(VonNeumannGridBuilder::default_cell_factory));

	MockGridCellFactory factory;
	VonNeumannGridBuilder builder (factory, 10, 10);
	ASSERT_THAT(builder.gridCellFactory(), Ref(factory));
}

class RandomAgentMappingTest : public Test {
	protected:
		NiceMock<MockDistribution<fpmas::model::DiscreteCoordinate>> mock_x;
		NiceMock<MockDistribution<fpmas::model::DiscreteCoordinate>> mock_y;

		// Actual cells that might be used as parameters for the countAt method
		std::array<NiceMock<MockGridCell>, 5> cells;
		std::array<fpmas::model::DiscreteCoordinate, 7> x_values {
			0, 4, 2, 0, 3, 4, 5
		};
		std::array<fpmas::model::DiscreteCoordinate, 7> y_values {
			1, 1, 0, 3, 2, 7, 1
		};
		// A set of cells corresponding to the x_values and y_values,
		// used to check that the total count of agents generated by the
		// mapping is 10
		std::array<NiceMock<MockGridCell>, 7> count_cells;

		void SetUp() override {
			ON_CALL(mock_x, call)
				.WillByDefault(ReturnRoundRobin({
							0, 4, 2, 0, 0, 3, 4, 3, 5, 0
							}));
			ON_CALL(mock_y, call)
				.WillByDefault(ReturnRoundRobin({
							1, 1, 0, 3, 1, 2, 7, 2, 1, 3
							}));

			// 2 agents in this cell
			ON_CALL(cells[0], location)
				.WillByDefault(Return(fpmas::model::DiscretePoint {0, 1}));
			// 1 agent
			ON_CALL(cells[1], location)
				.WillByDefault(Return(fpmas::model::DiscretePoint {4, 7}));
			// 0 agent
			ON_CALL(cells[2], location)
				.WillByDefault(Return(fpmas::model::DiscretePoint {6, 2}));
			// 2 agent
			ON_CALL(cells[3], location)
				.WillByDefault(Return(fpmas::model::DiscretePoint {3, 2}));
			// 1 agent
			ON_CALL(cells[4], location)
				.WillByDefault(Return(fpmas::model::DiscretePoint {5, 1}));

			for(std::size_t i = 0; i < 7; i++)
				ON_CALL(count_cells[i], location)
					.WillByDefault(Return(fpmas::model::DiscretePoint
								{x_values[i], y_values[i]}
								));
		}

};

TEST_F(RandomAgentMappingTest, test) {
	fpmas::model::RandomAgentMapping random_mapping(mock_x, mock_y, 10);

	ASSERT_EQ(random_mapping.countAt(&cells[0]), 2);
	ASSERT_EQ(random_mapping.countAt(&cells[1]), 1);
	ASSERT_EQ(random_mapping.countAt(&cells[2]), 0);
	ASSERT_EQ(random_mapping.countAt(&cells[3]), 2);
	ASSERT_EQ(random_mapping.countAt(&cells[4]), 1);

	std::size_t total_count = 0;
	for(auto& cell : count_cells)
		total_count+=random_mapping.countAt(&cell);
	ASSERT_EQ(total_count, 10);
}

class UniformAgentMappingTest : public Test {
	protected:
		fpmas::model::UniformAgentMapping mapping {10, 10, 30};
		std::array<NiceMock<MockGridCell>, 100> cells;

		void SetUp() override {
			for(fpmas::model::DiscreteCoordinate x = 0; x < 10; x++)
				for(fpmas::model::DiscreteCoordinate y = 0; y < 10; y++)
					ON_CALL(cells[10*x+y], location)
						.WillByDefault(Return(fpmas::model::DiscretePoint {x, y}));
		}
};

// Notice that an mpi test (in mpi/model/grid.cpp) checks the consistency of
// the map across processes. Then its enough to check reproducibility locally.
TEST_F(UniformAgentMappingTest, reproducibility) {
	fpmas::model::UniformAgentMapping other_mapping(10, 10, 30);
	for(std::size_t i = 0; i < 100; i++)
		ASSERT_EQ(mapping.countAt(&cells[i]), other_mapping.countAt(&cells[i]));
}
