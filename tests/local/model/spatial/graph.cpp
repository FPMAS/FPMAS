#include "fpmas/model/spatial/graph.h"
#include "model/mock_model.h"
#include "model/mock_spatial_model.h"

using namespace testing;

class TestGraphRange : public Test {
	protected:
		MockSpatialModel<fpmas::model::GraphCell, NiceMock> mock_model;
		MockAgentGraph<DefaultDistNode, DefaultDistEdge, NiceMock> mock_graph;
		NiceMock<MockAgentGroup> mock_cell_group;
		fpmas::model::GraphCell* center_cell;
		std::vector<fpmas::api::model::Agent*> neighbor_cells;
		std::vector<fpmas::api::model::AgentNode*> neighbor_nodes;

		void SetUp() override {
			ON_CALL(mock_model, graph())
				.WillByDefault(ReturnRef(mock_graph));
			ON_CALL(mock_model, cellGroup)
				.WillByDefault(ReturnRef(mock_cell_group));

			for(int i = 0; i < 4; i++) {
				fpmas::model::GraphCell* cell = new fpmas::model::GraphCell;
				auto node = new MockAgentNode<NiceMock>;
				ON_CALL(*node, getId)
					.WillByDefault(Return(fpmas::model::DistributedId(0, i)));
				cell->setNode(node);
				neighbor_nodes.push_back(node);
				neighbor_cells.push_back(cell);
			}

			center_cell = new fpmas::model::GraphCell;
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

		void TearDown() override {
			for(auto cell : neighbor_cells) {
				delete cell->node();
				delete cell;
			}
			delete center_cell->node();
			delete center_cell;
		}
};

TEST_F(TestGraphRange, include_location) {
	fpmas::model::GraphRange<> range(fpmas::model::INCLUDE_LOCATION);
	for(auto cell : neighbor_cells)
		ASSERT_TRUE(range.contains(
					center_cell, dynamic_cast<fpmas::model::GraphCell*>(cell)
					));
	ASSERT_TRUE(range.contains(center_cell, center_cell));
}

TEST_F(TestGraphRange, exclude_location) {
	fpmas::model::GraphRange<> range(fpmas::model::EXCLUDE_LOCATION);
	for(auto cell : neighbor_cells)
		ASSERT_TRUE(range.contains(
					center_cell, dynamic_cast<fpmas::model::GraphCell*>(cell)
					));
	ASSERT_FALSE(range.contains(center_cell, center_cell));
}
