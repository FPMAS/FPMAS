#include "../mocks/model/mock_environment.h"
#include "fpmas/model/environment.h"


using fpmas::model::CellBase;
using fpmas::model::AgentPtr;
using fpmas::api::model::EnvironmentLayers;

using namespace testing;

namespace environment {

	class CellBaseTestSetUp {
		protected:
			DistributedId _id_;
			NiceMock<MockLocatedAgent>* mock_located_agent = new NiceMock<MockLocatedAgent>;
			NiceMock<MockDistributedNode<AgentPtr>> agent_node {_id_, fpmas::model::AgentPtr(mock_located_agent)};
			NaggyMock<MockDistributedEdge<AgentPtr>> agent_edge;
			MockRange mock_mobility_range;
			MockRange mock_perception_range;

			NiceMock<MockDistributedNode<AgentPtr>> cell_node;

			NiceMock<MockCellBase>* cell_neighbor_1 = new NiceMock<MockCellBase>;
			NiceMock<MockDistributedNode<AgentPtr>> cell_neighbor_1_node {_id_, fpmas::model::AgentPtr(cell_neighbor_1)};
			NiceMock<MockDistributedEdge<AgentPtr>> cell_neighbor_1_edge;

			NiceMock<MockCellBase>* cell_neighbor_2 = new NiceMock<MockCellBase>;
			NiceMock<MockDistributedNode<AgentPtr>> cell_neighbor_2_node {_id_, fpmas::model::AgentPtr(cell_neighbor_2)};
			NiceMock<MockDistributedEdge<AgentPtr>> cell_neighbor_2_edge;

			StrictMock<MockModel> mock_model;

			void BaseTestSetUp(MockCellBase& cell) {
				ON_CALL(cell_node, getOutgoingEdges(EnvironmentLayers::NEIGHBOR_CELL))
					.WillByDefault(Return(
								std::vector<fpmas::model::AgentEdge*>
								{&cell_neighbor_1_edge, &cell_neighbor_2_edge}
								));
				ON_CALL(cell, model())
					.WillByDefault(Return(&mock_model));

				ON_CALL(cell, node()).WillByDefault(Return(&cell_node));
				ON_CALL(Const(cell), node()).WillByDefault(Return(&cell_node));

				agent_edge.setSourceNode(&agent_node);
				agent_edge.setTargetNode(&cell_node);

				cell_neighbor_1_edge.setTargetNode(&cell_neighbor_1_node);
				cell_neighbor_2_edge.setTargetNode(&cell_neighbor_2_node);

				ON_CALL(*mock_located_agent, mobilityRange)
					.WillByDefault(ReturnRef(mock_mobility_range));
				ON_CALL(*mock_located_agent, perceptionRange)
					.WillByDefault(ReturnRef(mock_perception_range));
			}

	};

	class CellBaseTest : public testing::Test, protected CellBaseTestSetUp, protected NiceMock<MockCellBase> {
		public:
			void SetUp() override {
				BaseTestSetUp(*this);
			}

			void rangeSetUp(bool move_to_new_location, bool perceive_new_location) {
				EXPECT_CALL(mock_mobility_range, contains(this))
					.WillRepeatedly(Return(move_to_new_location));
				EXPECT_CALL(mock_mobility_range, contains(cell_neighbor_1))
					.WillRepeatedly(Return(false));
				EXPECT_CALL(mock_mobility_range, contains(cell_neighbor_2))
					.WillRepeatedly(Return(true));
				EXPECT_CALL(mock_perception_range, contains(this))
					.WillRepeatedly(Return(perceive_new_location));
				EXPECT_CALL(mock_perception_range, contains(cell_neighbor_1))
					.WillRepeatedly(Return(true));
				EXPECT_CALL(mock_perception_range, contains(cell_neighbor_2))
					.WillRepeatedly(Return(false));
			}
	};

	TEST_F(CellBaseTest, grow_mobility_range) {
		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));

		rangeSetUp(true, true);

		this->growMobilityRange(mock_located_agent);
	}

	TEST_F(CellBaseTest, grow_perception_range) {
		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));

		rangeSetUp(true, true);

		this->growPerceptionRange(mock_located_agent);
	}

	TEST_F(CellBaseTest, update_location_1) {
		rangeSetUp(true, true);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::LOCATION));
		EXPECT_CALL(mock_model, unlink(&agent_edge));
		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::MOVE));
		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::PERCEIVE));

		fpmas::model::AgentPtr ptr {mock_located_agent};
		fpmas::model::Neighbor<fpmas::api::model::LocatedAgent> neighbor {&ptr, &agent_edge};

		this->updateLocation(neighbor);

		ptr.release();
	}

	TEST_F(CellBaseTest, update_location_2) {
		rangeSetUp(true, false);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::LOCATION));
		EXPECT_CALL(mock_model, unlink(&agent_edge));
		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::MOVE));

		fpmas::model::AgentPtr ptr {mock_located_agent};
		fpmas::model::Neighbor<fpmas::api::model::LocatedAgent> neighbor {&ptr, &agent_edge};

		this->updateLocation(neighbor);

		ptr.release();
	}

	TEST_F(CellBaseTest, update_location_3) {
		rangeSetUp(false, true);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::LOCATION));
		EXPECT_CALL(mock_model, unlink(&agent_edge));
		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::PERCEIVE));

		fpmas::model::AgentPtr ptr {mock_located_agent};
		fpmas::model::Neighbor<fpmas::api::model::LocatedAgent> neighbor {&ptr, &agent_edge};

		this->updateLocation(neighbor);

		ptr.release();
	}

	TEST_F(CellBaseTest, update_location_4) {
		rangeSetUp(false, false);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::LOCATION));
		EXPECT_CALL(mock_model, unlink(&agent_edge));

		fpmas::model::AgentPtr ptr {mock_located_agent};
		fpmas::model::Neighbor<fpmas::api::model::LocatedAgent> neighbor {&ptr, &agent_edge};

		this->updateLocation(neighbor);

		ptr.release();
	}

	TEST_F(CellBaseTest, update_ranges_new_location) {
		ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::NEW_LOCATION))
			.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

		rangeSetUp(true, true);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::LOCATION));
		EXPECT_CALL(mock_model, unlink(&agent_edge));
		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::MOVE));
		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::PERCEIVE));
		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));

		this->updateRanges();
	}

	TEST_F(CellBaseTest, update_ranges_new_move) {
		ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::NEW_MOVE))
			.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

		rangeSetUp(true, true);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::MOVE));
		EXPECT_CALL(mock_model, unlink(&agent_edge));
		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));

		this->updateRanges();
	}

	TEST_F(CellBaseTest, update_ranges_new_perceive) {
		ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::NEW_PERCEIVE))
			.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

		rangeSetUp(true, true);

		EXPECT_CALL(mock_model, link(mock_located_agent, this, EnvironmentLayers::PERCEIVE));
		EXPECT_CALL(mock_model, unlink(&agent_edge));

		EXPECT_CALL(mock_model, link(mock_located_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));

		this->updateRanges();
	}

	TEST_F(CellBaseTest, update_perceptions) {
		NiceMock<MockLocatedAgent>* perceived_agent = new NiceMock<MockLocatedAgent>;
		NiceMock<MockDistributedNode<AgentPtr>> perceived_agent_node {_id_, fpmas::model::AgentPtr(perceived_agent)};
		NiceMock<MockDistributedEdge<AgentPtr>> perceived_agent_edge;
		perceived_agent_edge.setSourceNode(&perceived_agent_node);

		ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::PERCEIVE))
			.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

		ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::LOCATION))
			.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&perceived_agent_edge})));

		EXPECT_CALL(mock_model, link(mock_located_agent, perceived_agent, EnvironmentLayers::PERCEIVE));
		EXPECT_CALL(mock_model, unlink(&agent_edge));

		this->updatePerceptions();
	}
}
