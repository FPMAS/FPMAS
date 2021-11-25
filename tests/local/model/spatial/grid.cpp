#include "fpmas/model/spatial/grid.h"
#include "fpmas/model/spatial/von_neumann.h"
#include "fpmas/model/spatial/moore.h"
#include "model/mock_grid.h"
#include "test_agents.h"

#include <unordered_set>

using namespace testing;
using namespace fpmas::model;

template<typename GridCellType>
class BaseGridCellTest : public testing::Test {
	protected:
		GridCellType grid_cell {{12, -7}};
		fpmas::api::model::AgentPtr src_ptr {&grid_cell};

		void TearDown() {
			src_ptr.release();
		}

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = fpmas::api::utils::PtrWrapper<typename GridCellType::JsonBase>(&grid_cell);
			}

		template<typename PackType>
			fpmas::api::utils::PtrWrapper<typename GridCellType::JsonBase> Unserialize(
					const PackType& pack) {
				return pack
					.template get<fpmas::api::utils::PtrWrapper<typename GridCellType::JsonBase>>();
			}
};

typedef BaseGridCellTest<model::test::GridCell> GridCellTest;

TEST_F(GridCellTest, location) {
	ASSERT_EQ(grid_cell.location(), DiscretePoint(12, -7));
}

#define GRID_CELL_SERIAL_TEST_SUITE(CELL_TYPE)\
	TEST_F(CELL_TYPE##Test, json) {\
		nlohmann::json j;\
		Serialize(j);\
		auto ptr = Unserialize(j);\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<CELL_TYPE*>(NotNull()));\
		ASSERT_EQ(\
				dynamic_cast<CELL_TYPE*>(ptr.get())->location(),\
				DiscretePoint(12, -7)\
				);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*dynamic_cast<CELL_TYPE*>(ptr.get()), grid_cell);\
		delete ptr.get();\
	}\
	\
	TEST_F(CELL_TYPE##Test, light_json) {\
		nlohmann::json j;\
		Serialize(j);\
		fpmas::io::json::light_json light_json;\
		Serialize(light_json);\
		auto ptr = Unserialize(light_json);\
		\
		ASSERT_LT(light_json.dump().size(), j.dump().size());\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<CELL_TYPE*>(NotNull()));\
		delete ptr.get();\
	}\
	\
	TEST_F(CELL_TYPE##Test, object_pack) {\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		auto ptr = Unserialize(pack);\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<CELL_TYPE*>(NotNull()));\
		ASSERT_EQ(\
				dynamic_cast<CELL_TYPE*>(ptr.get())->location(),\
				DiscretePoint(12, -7)\
				);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*dynamic_cast<CELL_TYPE*>(ptr.get()), grid_cell);\
		delete ptr.get();\
	}\
	\
	TEST_F(CELL_TYPE##Test, light_object_pack) {\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		fpmas::io::datapack::LightObjectPack light_pack;\
		Serialize(light_pack);\
		auto ptr = Unserialize(light_pack);\
		\
		ASSERT_LT(light_pack.data().size, pack.data().size);\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<CELL_TYPE*>(NotNull()));\
		delete ptr.get();\
	}\


typedef BaseGridCellTest<model::test::CustomGridCell> CustomGridCellTest;

namespace {
	using model::test::GridCell;
	/*
	 * Serialization tests for the default fpmas::model::GridCell, serialized using
	 * FPMAS_DEFAULT_JSON() and FPMAS_DEFAULT_DATAPACK().
	 */
	GRID_CELL_SERIAL_TEST_SUITE(GridCell);

	using model::test::CustomGridCell;
	/*
	 * Serialization tests for an fpmas::model::GridCellBase extension with
	 * custom serialization rules
	 */
	GRID_CELL_SERIAL_TEST_SUITE(CustomGridCell);
}


/*
 * Inheriting from GridAgentType is an hacky trick to get access to protected
 * GridAgentType members (such as moveTo())
 */
template<typename GridAgentType>
class BaseGridAgentTest : public testing::Test, protected GridAgentType {
	protected:
		GridAgentType& grid_agent {*this};
		MockAgentNode<NiceMock> mock_agent_node {{0, 0}};

		MockGridCell<NiceMock> mock_cell;
		fpmas::graph::DistributedId mock_cell_id {37, 2};
		MockAgentNode<NiceMock> mock_cell_node {mock_cell_id, &mock_cell};
		DiscretePoint location_point {3, -18};
		NiceMock<MockModel> mock_model;

		static void SetUpTestSuite() {
			GridAgentType::mobility_range
				= new NiceMock<MockRange<MockGridCell<NiceMock>>>;
			GridAgentType::perception_range
				= new NiceMock<MockRange<MockGridCell<NiceMock>>>;
		}
		static void TearDownTestSuite() {
			delete GridAgentType::mobility_range;
			delete GridAgentType::perception_range;
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

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = 
					fpmas::api::utils::PtrWrapper<typename GridAgentType::JsonBase>(&grid_agent);
			}

		template<typename PackType>
			fpmas::api::utils::PtrWrapper<typename GridAgentType::JsonBase> Unserialize(const PackType& pack) {
				return pack
					.template get<fpmas::api::utils::PtrWrapper<typename GridAgentType::JsonBase>>();
			}
};

typedef BaseGridAgentTest<model::test::GridAgent> GridAgentTest;

TEST_F(GridAgentTest, location) {
	grid_agent.initLocation(&mock_cell);

	ASSERT_EQ(grid_agent.locationPoint(), location_point);
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

#define GRID_AGENT_SERIAL_TEST_SUITE(AGENT_TYPE)\
	TEST_F(AGENT_TYPE##Test, json) {\
		grid_agent.initLocation(&mock_cell);\
		nlohmann::json j;\
		Serialize(j);\
		\
		auto unserialized_agent = Unserialize(j);\
		\
		ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		ASSERT_EQ(\
				static_cast<AGENT_TYPE*>(unserialized_agent.get())->locationId(),\
				mock_cell_id);\
		ASSERT_EQ(\
				static_cast<AGENT_TYPE*>(unserialized_agent.get())->locationPoint(),\
				location_point);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*static_cast<AGENT_TYPE*>(unserialized_agent.get()), grid_agent);\
		\
		delete unserialized_agent.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, light_json) {\
		grid_agent.initLocation(&mock_cell);\
		nlohmann::json j;\
		Serialize(j);\
		\
		fpmas::io::json::light_json light_json;\
		Serialize(light_json);\
		\
		ASSERT_LT(light_json.dump().size(), j.dump().size());\
		\
		auto unserialized_agent = Unserialize(light_json);\
		\
		ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		\
		delete unserialized_agent.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, object_pack) {\
		grid_agent.initLocation(&mock_cell);\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		\
		auto unserialized_agent = Unserialize(pack);\
		\
		ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		ASSERT_EQ(\
				static_cast<AGENT_TYPE*>(unserialized_agent.get())->locationId(),\
				mock_cell_id);\
		ASSERT_EQ(\
				static_cast<AGENT_TYPE*>(unserialized_agent.get())->locationPoint(),\
				location_point);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*static_cast<AGENT_TYPE*>(unserialized_agent.get()), grid_agent);\
		\
		delete unserialized_agent.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, light_object_pack) {\
		grid_agent.initLocation(&mock_cell);\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		\
		fpmas::io::datapack::LightObjectPack light_pack;\
		Serialize(light_pack);\
		\
		ASSERT_LT(light_pack.data().size, pack.data().size);\
		\
		auto unserialized_agent = Unserialize(light_pack);\
		\
		ASSERT_THAT(unserialized_agent.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		\
		delete unserialized_agent.get();\
	}\

namespace {
	using model::test::GridAgent;
	/*
	 * Serialization tests for an fpmas::model::GridAgent extension, serialized
	 * using FPMAS_DEFAULT_JSON() and FPMAS_DEFAULT_DATAPACK().
	 */
	GRID_AGENT_SERIAL_TEST_SUITE(GridAgent);

	using model::test::GridAgentWithData;
	typedef BaseGridAgentTest<GridAgentWithData> GridAgentWithDataTest;

	/*
	 * Serialization tests for an fpmas::model::GridAgent
	 * extension with custom serialization rules.
	 */
	GRID_AGENT_SERIAL_TEST_SUITE(GridAgentWithData);
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
