#include "../mocks/model/mock_spatial_model.h"
#include "../mocks/communication/mock_communication.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "gtest_environment.h"


using fpmas::api::model::SpatialModelLayers;

using namespace testing;
using namespace fpmas::model;

/*
 * MockCellBase inherits from fpmas::model::CellBase, so that methods defined
 * in fpmas::api::model::Cell are implemented, but other
 * fpmas::api::model::Agent methods are mocked.
 * See mocks/model/mock_environment.h.
 */

class CellBaseTest : public Test {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;
		MockCellBase mock_cell;

		DistributedId agent_id {7, 4};
		MockSpatialAgent<DefaultCell, NiceMock>* mock_spatial_agent
			= new NiceMock<MockSpatialAgent<DefaultCell, NiceMock>>;
		MockCell<NiceMock> current_location;
		MockAgentNode<NiceMock> agent_node {
			agent_id, AgentPtr(mock_spatial_agent)};
		MockAgentEdge<NiceMock> agent_edge;

		MockAgentNode<NiceMock> cell_node;

		MockCellBase* cell_neighbor_1 = new MockCellBase;
		MockAgentNode<NiceMock> cell_neighbor_1_node {
			agent_id, AgentPtr(cell_neighbor_1)};
		MockAgentEdge<NiceMock> cell_neighbor_1_edge;

		MockCellBase* cell_neighbor_2 = new MockCellBase;
		MockAgentNode<NiceMock> cell_neighbor_2_node {
			agent_id, AgentPtr(cell_neighbor_2)};
		MockAgentEdge<NiceMock> cell_neighbor_2_edge;

		StrictMock<MockModel> mock_model;

		void SetUp() override {
			ON_CALL(cell_node, getOutgoingEdges(SpatialModelLayers::CELL_SUCCESSOR))
				.WillByDefault(Return(
							std::vector<AgentEdge*>
							{&cell_neighbor_1_edge, &cell_neighbor_2_edge}
							));
			ON_CALL(mock_cell, model())
				.WillByDefault(Return(&mock_model));

			ON_CALL(mock_cell, node()).WillByDefault(Return(&cell_node));
			ON_CALL(Const(mock_cell), node()).WillByDefault(Return(&cell_node));

			agent_edge.setSourceNode(&agent_node);
			agent_edge.setTargetNode(&cell_node);

			cell_neighbor_1_edge.setTargetNode(&cell_neighbor_1_node);
			cell_neighbor_2_edge.setTargetNode(&cell_neighbor_2_node);

			ON_CALL(*mock_spatial_agent, locationCell)
				.WillByDefault(Return(&current_location));
			ON_CALL(*mock_spatial_agent, node())
				.WillByDefault(Return(&agent_node));
		}
};

TEST_F(CellBaseTest, handle_new_location) {
	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::NEW_LOCATION))
		.WillByDefault(Return(
					std::vector<AgentEdge*>({&agent_edge})
					));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, &mock_cell, SpatialModelLayers::LOCATION));
	EXPECT_CALL(mock_model, unlink(&agent_edge));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_1, SpatialModelLayers::NEW_MOVE));
	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_2, SpatialModelLayers::NEW_MOVE));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_1, SpatialModelLayers::NEW_PERCEIVE)
			);
	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_2, SpatialModelLayers::NEW_PERCEIVE)
			);

	mock_cell.handleNewLocation();
}

TEST_F(CellBaseTest, handle_move) {
	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::MOVE))
		.WillByDefault(Return(std::vector<AgentEdge*>({&agent_edge})));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_1, SpatialModelLayers::NEW_MOVE));
	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_2, SpatialModelLayers::NEW_MOVE));
	mock_cell.handleMove();

	// Should do nothing when called again, since the node is already
	// explored
	EXPECT_CALL(mock_model, link).Times(0);
	mock_cell.handleMove();
}

TEST_F(CellBaseTest, handle_perceive) {
	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::PERCEIVE))
		.WillByDefault(Return(std::vector<AgentEdge*>({&agent_edge})));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_1, SpatialModelLayers::NEW_PERCEIVE)
			);
	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, cell_neighbor_2, SpatialModelLayers::NEW_PERCEIVE)
			);
	mock_cell.handlePerceive();

	// Should do nothing when called again, since the node is already
	// explored
	EXPECT_CALL(mock_model, link).Times(0);
	mock_cell.handlePerceive();
}

TEST_F(CellBaseTest, update_perceptions) {
	DistributedId perceived_id {5, 2};
	MockSpatialAgent<DefaultCell, NiceMock>* perceived_agent
		= new MockSpatialAgent<DefaultCell, NiceMock>;

	// TODO: this set up might easily broke if updatePerceptions implementation
	// changes
	NiceMock<MockAgentGroup> mock_group;
	ON_CALL(mock_group, groupId())
		.WillByDefault(Return(0));
	ON_CALL(*perceived_agent, groupIds)
		.WillByDefault(Return(std::vector<fpmas::model::GroupId> {mock_group.groupId()}));
	ON_CALL(*mock_spatial_agent, groupIds)
		.WillByDefault(Return(std::vector<fpmas::model::GroupId> {mock_group.groupId()}));

	MockAgentNode<NiceMock> perceived_agent_node {
		perceived_id, AgentPtr(perceived_agent)};
	ON_CALL(*perceived_agent, node())
		.WillByDefault(Return(&perceived_agent_node));
	MockAgentEdge<NiceMock> perceived_agent_edge;
	perceived_agent_edge.setSourceNode(&perceived_agent_node);

	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::PERCEIVE))
		.WillByDefault(Return(std::vector<AgentEdge*>({&agent_edge})));

	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentEdge*>(
						{&agent_edge, &perceived_agent_edge}
						)));

	EXPECT_CALL(mock_model, link(
				mock_spatial_agent, perceived_agent, SpatialModelLayers::PERCEPTION));

	mock_cell.updatePerceptions(mock_group);
}

class SpatialAgentTest : public ::testing::Test, protected model::test::SpatialAgent {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;

		StrictMock<MockModel> mock_model;
		fpmas::api::model::SpatialAgent<DefaultCell>& agent;
		MockAgentNode<NiceMock> agent_node {{2, 37}};

		fpmas::graph::DistributedId location_id {12, 67};
		MockCell<NiceMock> location_cell;
		MockAgentEdge<NiceMock> location_edge;
		MockAgentNode<NiceMock> location_node {location_id, &location_cell};

		SpatialAgentTest() : agent(*this) {}


		void SetUp() override {
			this->setNode(&agent_node);
			this->setModel(&mock_model);

			location_edge.setSourceNode(&agent_node);
			location_edge.setTargetNode(&location_node);

			ON_CALL(location_cell, node())
				.WillByDefault(Return(&location_node));
		}

		void TearDown() override {
			location_node.data().release();
		}

		void setUpRandomRange(
				std::mt19937_64& engine,
				std::vector<AgentNode*>& nodes,
				std::vector<AgentEdge*>& edges,
				std::array<bool, 10>& in_range,
				MockRange<DefaultCell>& range
				) {
			std::bernoulli_distribution dist(.5);

			EXPECT_CALL(range, contains).Times(AnyNumber());
			for(std::size_t i = 0; i < 10; i++) {
				edges[i]->setSourceNode(&agent_node);
				edges[i]->setTargetNode(nodes[i]);
				in_range[i] = dist(engine);
				ON_CALL(range, contains(
							&location_cell,
							dynamic_cast<fpmas::api::model::Cell*>(
								nodes[i]->data().get())
							)).WillByDefault(Return(in_range[i]));
			}
		}

		template<template<typename> class Strictness>
		void setUpDuplicates(
				MockCell<Strictness>* cell,
				std::vector<AgentNode*>& nodes,
				std::vector<AgentEdge*>& edges,
				fpmas::graph::LayerId layer,
				MockRange<DefaultCell>& range) {

			auto node = new MockAgentNode<NiceMock>({0, 1}, cell);
			ON_CALL(*cell, node())
				.WillByDefault(Return(node));
			nodes.push_back(node);
			nodes.push_back(node);

			for(int i = 0; i < 2; i++) {
				auto edge = new MockAgentEdge<NiceMock>;
				edge->setSourceNode(&agent_node);
				edge->setTargetNode(node);
				edges.push_back(edge);
			}

			ON_CALL(agent_node, outNeighbors(layer))
				.WillByDefault(Return(nodes));
			ON_CALL(agent_node, getOutgoingEdges(layer))
				.WillByDefault(Return(edges));

			EXPECT_CALL(range, contains(_, cell))
				.WillRepeatedly(Return(true));

		}
};

TEST_F(SpatialAgentTest, location) {
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentEdge*> {
					&location_edge}));
	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentNode*> {
					&location_node}));

	ASSERT_EQ(this->locationCell(), &location_cell);
}

TEST_F(SpatialAgentTest, locationId) {
	EXPECT_CALL(mock_model, link)
		.Times(AnyNumber());
	this->initLocation(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, json) {
	EXPECT_CALL(mock_model, link).Times(AnyNumber());
	this->initLocation(&location_cell);

	fpmas::api::model::AgentPtr src_ptr (this);
	nlohmann::json j = src_ptr;

	auto ptr = j.get<fpmas::api::model::AgentPtr>();

	ASSERT_THAT(ptr.get(), WhenDynamicCastTo<SpatialAgent*>(NotNull()));
	ASSERT_EQ(dynamic_cast<SpatialAgent*>(ptr.get())->locationId(), location_id);

	src_ptr.release();
}

TEST_F(SpatialAgentTest, json_with_data) {
	EXPECT_CALL(mock_model, link).Times(AnyNumber());
	// SetUp used only for this test
	NiceMock<model::test::SpatialAgentWithData> agent;
	AgentPtr src_ptr (&agent);
	agent.setModel(&mock_model);
	agent.setNode(&agent_node);

	agent.data= 7;
	agent.initLocation(&location_cell);

	// Serialization
	nlohmann::json j = src_ptr;

	// Unserialization
	auto ptr = j.get<AgentPtr>();

	ASSERT_THAT(ptr.get(), WhenDynamicCastTo<model::test::SpatialAgentWithData*>(NotNull()));
	ASSERT_EQ(dynamic_cast<model::test::SpatialAgentWithData*>(ptr.get())->data, 7);
	ASSERT_EQ(
			dynamic_cast<model::test::SpatialAgentWithData*>(ptr.get())->locationId(),
			location_id);

	src_ptr.release();
}

TEST_F(SpatialAgentTest, moveToCell1) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));

	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::MOVE));
	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::PERCEIVE));
	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell2) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));

	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::PERCEIVE));
	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell3) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));

	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::MOVE));
	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell4) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));

	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToId) {
	// Build MOVE layer
	std::vector<AgentNode*> move_neighbors {&location_node};
	std::vector<AgentEdge*> move_neighbor_edges {&location_edge};

	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	EXPECT_CALL(mock_model, link(
				&agent, &location_cell, SpatialModelLayers::NEW_LOCATION));
	EXPECT_CALL(mock_model, unlink(&location_edge));

	this->moveTo(location_id);
}

TEST_F(SpatialAgentTest, moveToIdOutOfField) {
	ASSERT_THROW(
			this->moveTo(location_id),
			fpmas::api::model::OutOfMobilityFieldException);
}

TEST_F(SpatialAgentTest, agentBehavior) {
	std::mt19937_64 engine;

	// Build NEW_MOVE layer
	std::vector<AgentNode*> move_neighbors;
	std::vector<AgentEdge*> move_neighbor_edges;
	for(FPMAS_ID_TYPE i = 0; i < 10; i++) {
		auto cell = new MockCell<NiceMock>;
		auto node = new MockAgentNode<NiceMock>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		move_neighbors.push_back(node);
		move_neighbor_edges.push_back(new MockAgentEdge<NiceMock>);
	}

	std::array<bool, 10> in_move_range;
	setUpRandomRange(
			engine, move_neighbors, move_neighbor_edges,
			in_move_range, this->mobility_range);

	// Build NEW_PERCEIVE layer
	std::vector<AgentNode*> perceive_neighbors;
	std::vector<AgentEdge*> perceive_neighbor_edges;
	for(FPMAS_ID_TYPE i = 0; i < 10; i++) {
		auto cell = new MockCell<NiceMock>;
		auto node = new MockAgentNode<NiceMock>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		perceive_neighbors.push_back(node);
		perceive_neighbor_edges.push_back(new MockAgentEdge<NiceMock>);
	}
	std::array<bool, 10> in_perceive_range;
	setUpRandomRange(
			engine, perceive_neighbors, perceive_neighbor_edges,
			in_perceive_range, this->perception_range);

	// Set up NEW_MOVE layer
	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::NEW_MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::NEW_MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	// Set up NEW_PERCEIVE layer
	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::NEW_PERCEIVE))
		.WillByDefault(Return(perceive_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::NEW_PERCEIVE))
		.WillByDefault(Return(perceive_neighbor_edges));

	// Set up location
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentEdge*> {
					&location_edge}));
	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentNode*> {
					&location_node}));

	// MOVE expectations
	for(int i = 0; i < 10; i++) {
		if(in_move_range[i])
			EXPECT_CALL(mock_model, link(
						&agent, move_neighbors[i]->data().get(),
						SpatialModelLayers::MOVE));
		EXPECT_CALL(mock_model, unlink(move_neighbor_edges[i]));
	}
	this->handleNewMove();

	// PERCEIVE expectations
	for(int i = 0; i < 10; i++) {
		if(in_perceive_range[i])
			EXPECT_CALL(mock_model, link(
						&agent, perceive_neighbors[i]->data().get(),
						SpatialModelLayers::PERCEIVE));
		EXPECT_CALL(mock_model, unlink(perceive_neighbor_edges[i]));
	}
	this->handleNewPerceive();

	for(auto node : move_neighbors)
		delete node;
	for(auto edge : move_neighbor_edges)
		delete edge;

	for(auto node : perceive_neighbors)
		delete node;
	for(auto edge : perceive_neighbor_edges)
		delete edge;
}

/*
 * While exploring the graph, because of cycles, the same node might be added
 * several times in the NEW_MOVE or NEW_PERCEIVE layer at the same iteration.
 * In this case, the corresponding node must be linked **exactly once** on the
 * MOVE or PERCEIVE layer, and **all** corresponding edges must be unlinked.
 */
TEST_F(SpatialAgentTest, agentBehaviorWithDuplicates) {
	// Set up move layer with a duplicate node
	auto move_cell = new MockCell<NiceMock>;
	std::vector<AgentNode*> move_neighbors;
	std::vector<AgentEdge*> move_neighbor_edges;
	setUpDuplicates(
			move_cell, move_neighbors, move_neighbor_edges,
			SpatialModelLayers::NEW_MOVE, this->mobility_range);

	// Set up perceive layer with a duplicate node
	auto perceive_cell = new MockCell<NiceMock>;
	std::vector<AgentNode*> perceive_neighbors;
	std::vector<AgentEdge*> perceive_neighbor_edges;
	setUpDuplicates(
			perceive_cell, perceive_neighbors, perceive_neighbor_edges,
			SpatialModelLayers::NEW_PERCEIVE, this->perception_range);

	EXPECT_CALL(mock_model, link(&agent, move_cell, SpatialModelLayers::MOVE))
		.Times(1);
	for(auto edge : move_neighbor_edges)
		EXPECT_CALL(mock_model, unlink(edge));

	this->handleNewMove();

	EXPECT_CALL(mock_model, link(&agent, perceive_cell, SpatialModelLayers::PERCEIVE))
		.Times(1);
	for(auto edge : perceive_neighbor_edges)
		EXPECT_CALL(mock_model, unlink(edge));

	this->handleNewPerceive();

	delete move_neighbors[0];
	for(auto edge : move_neighbor_edges)
		delete edge;
	delete perceive_neighbors[0];
	for(auto edge : perceive_neighbor_edges)
		delete edge;
}

class EndConditionTest : public Test {
	protected:
		class Range : public NiceMock<MockRange<MockCell<>>> {
			public:
				int size;
				Range(int size)
					: size(size) {
						ON_CALL(*this, radius)
							.WillByDefault(ReturnPointee(&this->size));
					}
		};
		MockMpiCommunicator<> comm;
};
TEST_F(EndConditionTest, static_end_condition) {
	fpmas::model::StaticEndCondition<Range, 4, MockCell<>> end_condition;
	end_condition.init(comm, {}, {});

	std::size_t counter = 0;
	while(!end_condition.end()) {
		end_condition.step();
		counter++;
	}

	ASSERT_EQ(counter, 4);
}
