#include "fpmas/model/spatial/graph.h"
#include "model/mock_model.h"
#include "model/mock_spatial_model.h"
#include "test_agents.h"

using namespace testing;

template<typename CellType>
class BaseGraphCellTest : public Test {
	protected:
		MockSpatialModel<fpmas::model::GraphCell, NiceMock> mock_model;
		MockAgentGraph<DefaultDistNode, DefaultDistEdge, NiceMock> mock_graph;
		NiceMock<MockAgentGroup> mock_cell_group;
		CellType* center_cell;
		std::vector<fpmas::api::model::Agent*> neighbor_cells;
		std::vector<fpmas::api::model::AgentNode*> neighbor_nodes;

		void SetUp() override {
			ON_CALL(mock_model, graph())
				.WillByDefault(ReturnRef(mock_graph));
			ON_CALL(mock_model, cellGroup())
				.WillByDefault(ReturnRef(mock_cell_group));
			ON_CALL(Const(mock_model), cellGroup())
				.WillByDefault(ReturnRef(mock_cell_group));

			for(int i = 0; i < 4; i++) {
				CellType* cell = new CellType;
				auto node = new MockAgentNode<NiceMock>;
				ON_CALL(*node, getId)
					.WillByDefault(Return(fpmas::model::DistributedId(0, i)));
				cell->setNode(node);
				neighbor_nodes.push_back(node);
				neighbor_cells.push_back(cell);
			}

			center_cell = new CellType;
			auto node = new MockAgentNode<NiceMock>;
			ON_CALL(*node, getId)
				.WillByDefault(Return(fpmas::model::DistributedId(0, 4)));
			center_cell->setNode(node);
			ON_CALL(*node, outNeighbors(fpmas::api::model::CELL_SUCCESSOR))
				.WillByDefault(Return(neighbor_nodes));

			ON_CALL(mock_cell_group, localAgents)
				.WillByDefault(Return(
							std::vector<fpmas::api::model::Agent*>({center_cell})
							));

			fpmas::model::GraphRange<>::synchronize(mock_model);
		}

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = fpmas::api::utils::PtrWrapper<typename CellType::JsonBase>(
						center_cell);
			}

		template<typename PackType>
			fpmas::api::utils::PtrWrapper<typename CellType::JsonBase> Unserialize(
					const PackType& pack) {
				return pack
					.template get<fpmas::api::utils::PtrWrapper<typename CellType::JsonBase>>();
			}

		void TearDown() override {
			for(auto cell : neighbor_cells) {
				delete cell->node();
				delete cell;
			}
			delete center_cell->node();
			delete center_cell;
		}
};

typedef BaseGraphCellTest<fpmas::model::GraphCell> GraphCellTest;

TEST_F(GraphCellTest, graph_range_include_location) {
	fpmas::model::GraphRange<> range(fpmas::model::INCLUDE_LOCATION);
	for(auto cell : neighbor_cells)
		ASSERT_TRUE(range.contains(
					center_cell, dynamic_cast<fpmas::model::GraphCell*>(cell)
					));
	ASSERT_TRUE(range.contains(center_cell, center_cell));
}

TEST_F(GraphCellTest, graph_range_exclude_location) {
	fpmas::model::GraphRange<> range(fpmas::model::EXCLUDE_LOCATION);
	for(auto cell : neighbor_cells)
		ASSERT_TRUE(range.contains(
					center_cell, dynamic_cast<fpmas::model::GraphCell*>(cell)
					));
	ASSERT_FALSE(range.contains(center_cell, center_cell));
}

#define GRAPH_CELL_SERIAL_TEST_SUITE(CELL_TYPE)\
	TEST_F(CELL_TYPE##Test, json) {\
		nlohmann::json j;\
		Serialize(j);\
		auto ptr = Unserialize(j);\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<CELL_TYPE*>(NotNull()));\
		ASSERT_THAT(\
				dynamic_cast<CELL_TYPE*>(ptr.get())->reachableCells(),\
				UnorderedElementsAre(\
					DistributedId(0, 0), DistributedId(0, 1),\
					DistributedId(0, 2), DistributedId(0, 3)\
					)\
				);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*dynamic_cast<CELL_TYPE*>(ptr.get()), *center_cell);\
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
		ASSERT_THAT(\
				dynamic_cast<CELL_TYPE*>(ptr.get())->reachableCells(),\
				UnorderedElementsAre(\
					DistributedId(0, 0), DistributedId(0, 1),\
					DistributedId(0, 2), DistributedId(0, 3)\
					)\
				);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*dynamic_cast<CELL_TYPE*>(ptr.get()), *center_cell);\
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


typedef BaseGraphCellTest<model::test::CustomGraphCell> CustomGraphCellTest;

namespace {
	using model::test::GraphCell;
	/*
	 * Serialization tests for the default fpmas::model::GraphCell, serialized using
	 * FPMAS_DEFAULT_JSON() and FPMAS_DEFAULT_DATAPACK().
	 */
	GRAPH_CELL_SERIAL_TEST_SUITE(GraphCell);

	using model::test::CustomGraphCell;
	/*
	 * Serialization tests for an fpmas::model::GraphCellBase extension with
	 * custom serialization rules
	 */
	GRAPH_CELL_SERIAL_TEST_SUITE(CustomGraphCell);
}
