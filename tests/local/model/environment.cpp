#include "../mocks/model/mock_environment.h"
#include "fpmas/model/environment.h"
#include "gtest_environment.h"


using fpmas::model::CellBase;
using fpmas::model::AgentPtr;
using fpmas::api::model::EnvironmentLayers;
// defined in gtest_environment
using model::test::SpatialAgent;
using model::test::SpatialAgentWithData;

using namespace testing;

class CellBaseTestSetUp {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;

		DistributedId agent_id {7, 4};
		NiceMock<MockSpatialAgent>* mock_spatial_agent
			= new NiceMock<MockSpatialAgent>;
		NiceMock<MockCell> current_location;
		NiceMock<MockDistributedNode<AgentPtr>> agent_node {agent_id, fpmas::model::AgentPtr(mock_spatial_agent)};
		NaggyMock<MockDistributedEdge<AgentPtr>> agent_edge;

		NiceMock<MockDistributedNode<AgentPtr>> cell_node;

		NiceMock<MockCellBase>* cell_neighbor_1 = new NiceMock<MockCellBase>;
		NiceMock<MockDistributedNode<AgentPtr>> cell_neighbor_1_node {agent_id, fpmas::model::AgentPtr(cell_neighbor_1)};
		NiceMock<MockDistributedEdge<AgentPtr>> cell_neighbor_1_edge;

		NiceMock<MockCellBase>* cell_neighbor_2 = new NiceMock<MockCellBase>;
		NiceMock<MockDistributedNode<AgentPtr>> cell_neighbor_2_node {agent_id, fpmas::model::AgentPtr(cell_neighbor_2)};
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

			ON_CALL(*mock_spatial_agent, locationCell)
				.WillByDefault(Return(&current_location));
			ON_CALL(*mock_spatial_agent, node())
				.WillByDefault(Return(&agent_node));
		}

};

/*
 * MockCellBase inherits from fpmas::model::CellBase, so that methods defined
 * in fpmas::api::model::Cell are implemented, but other
 * fpmas::api::model::Agent methods are mocked.
 * See mocks/model/mock_environment.h.
 */

class CellBaseTest : public testing::Test, protected CellBaseTestSetUp,
	// The test directly inherits from MockCellBase, to have access to
	// fpmas::model::CellBase protected methods that need to be tested
	protected NiceMock<MockCellBase> {
	public:
		void SetUp() override {
			BaseTestSetUp(*this);
		}
};

TEST_F(CellBaseTest, grow_mobility_range) {
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_MOVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));

	this->growMobilityRange(mock_spatial_agent);
}

TEST_F(CellBaseTest, grow_perception_range) {
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_PERCEIVE));

	this->growPerceptionRange(mock_spatial_agent);
}

TEST_F(CellBaseTest, update_location) {
	EXPECT_CALL(mock_model, link(mock_spatial_agent, this, EnvironmentLayers::LOCATION));
	EXPECT_CALL(mock_model, unlink(&agent_edge));

	fpmas::model::AgentPtr ptr {mock_spatial_agent};
	fpmas::model::Neighbor<fpmas::api::model::Agent> neighbor {&ptr, &agent_edge};

	this->updateLocation(neighbor);

	ptr.release();
}

TEST_F(CellBaseTest, handle_new_location) {
	ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::NEW_LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, this, EnvironmentLayers::LOCATION));
	EXPECT_CALL(mock_model, unlink(&agent_edge));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_MOVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_PERCEIVE));

	this->handleNewLocation();
}

TEST_F(CellBaseTest, handle_move) {
	ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::MOVE))
		.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_MOVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_MOVE));
	this->handleMove();

	// Should do nothing when called again, since the node is already
	// explored
	EXPECT_CALL(mock_model, link).Times(0);
	this->handleMove();
}

TEST_F(CellBaseTest, handle_perceive) {
	ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::PERCEIVE))
		.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_1, EnvironmentLayers::NEW_PERCEIVE));
	EXPECT_CALL(mock_model, link(mock_spatial_agent, cell_neighbor_2, EnvironmentLayers::NEW_PERCEIVE));
	this->handlePerceive();

	// Should do nothing when called again, since the node is already
	// explored
	EXPECT_CALL(mock_model, link).Times(0);
	this->handlePerceive();
}

TEST_F(CellBaseTest, update_perceptions) {
	DistributedId perceived_id {5, 2};
	NiceMock<MockSpatialAgent>* perceived_agent
		= new NiceMock<MockSpatialAgent>;
	NiceMock<MockDistributedNode<AgentPtr>> perceived_agent_node {perceived_id, fpmas::model::AgentPtr(perceived_agent)};
	ON_CALL(*perceived_agent, node())
		.WillByDefault(Return(&perceived_agent_node));
	NiceMock<MockDistributedEdge<AgentPtr>> perceived_agent_edge;
	perceived_agent_edge.setSourceNode(&perceived_agent_node);

	ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::PERCEIVE))
		.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge})));

	ON_CALL(cell_node, getIncomingEdges(EnvironmentLayers::LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>({&agent_edge, &perceived_agent_edge})));

	EXPECT_CALL(mock_model, link(mock_spatial_agent, perceived_agent, EnvironmentLayers::PERCEPTION));
	EXPECT_CALL(mock_model, unlink(&agent_edge));

	this->updatePerceptions();
}

class SpatialAgentTest : public ::testing::Test, protected NiceMock<SpatialAgent> {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;

		StrictMock<MockModel> mock_model;
		fpmas::api::model::SpatialAgent& agent;
		MockDistributedNode<AgentPtr> agent_node {{2, 37}};

		fpmas::graph::DistributedId location_id {12, 67};
		MockCell location_cell;
		MockDistributedEdge<AgentPtr> location_edge;
		MockDistributedNode<AgentPtr> location_node {location_id, &location_cell};

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
				std::vector<fpmas::model::AgentNode*>& nodes,
				std::vector<fpmas::model::AgentEdge*>& edges,
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
							dynamic_cast<fpmas::api::model::Cell*>(nodes[i]->data().get())))
					.WillByDefault(Return(in_range[i]));
			}
		}

		void setUpDuplicates(
				MockCell* cell,
				std::vector<fpmas::model::AgentNode*>& nodes,
				std::vector<fpmas::model::AgentEdge*>& edges,
				fpmas::graph::LayerId layer,
				MockRange<DefaultCell>& range) {

			auto node = new MockDistributedNode<AgentPtr>({0, 1}, cell);
			ON_CALL(*cell, node())
				.WillByDefault(Return(node));
			nodes.push_back(node);
			nodes.push_back(node);

			for(int i = 0; i < 2; i++) {
				auto edge = new MockDistributedEdge<AgentPtr>;
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
	ON_CALL(agent_node, getOutgoingEdges(EnvironmentLayers::LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::model::AgentEdge*> {&location_edge}));
	ON_CALL(agent_node, outNeighbors(EnvironmentLayers::LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::model::AgentNode*> {&location_node}));

	ASSERT_EQ(this->locationCell(), &location_cell);
}

TEST_F(SpatialAgentTest, locationId) {
	EXPECT_CALL(mock_model, link).Times(AnyNumber());
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
	NiceMock<SpatialAgentWithData> agent;
	fpmas::model::AgentPtr src_ptr (&agent);
	agent.setModel(&mock_model);
	agent.setNode(&agent_node);

	agent.data= 7;
	agent.initLocation(&location_cell);

	// Serialization
	nlohmann::json j = src_ptr;

	// Unserialization
	auto ptr = j.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(ptr.get(), WhenDynamicCastTo<SpatialAgentWithData*>(NotNull()));
	ASSERT_EQ(dynamic_cast<SpatialAgentWithData*>(ptr.get())->data, 7);
	ASSERT_EQ(dynamic_cast<SpatialAgentWithData*>(ptr.get())->locationId(), location_id);

	src_ptr.release();
}

TEST_F(SpatialAgentTest, moveToCell1) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));

	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::MOVE));
	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::PERCEIVE));
	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell2) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));

	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::PERCEIVE));
	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell3) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(true));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));

	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::MOVE));
	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToCell4) {
	EXPECT_CALL(this->mobility_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));
	EXPECT_CALL(this->perception_range, contains(&location_cell, &location_cell))
		.WillRepeatedly(Return(false));

	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::NEW_LOCATION));

	this->moveTo(&location_cell);

	ASSERT_EQ(this->locationId(), location_id);
}

TEST_F(SpatialAgentTest, moveToId) {
	// Build MOVE layer
	std::vector<fpmas::model::AgentNode*> move_neighbors {&location_node};
	std::vector<fpmas::model::AgentEdge*> move_neighbor_edges {&location_edge};

	ON_CALL(agent_node, outNeighbors(EnvironmentLayers::MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(EnvironmentLayers::MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	EXPECT_CALL(mock_model, link(&agent, &location_cell, EnvironmentLayers::NEW_LOCATION));
	EXPECT_CALL(mock_model, unlink(&location_edge));

	this->moveTo(location_id);
}

TEST_F(SpatialAgentTest, moveToIdOutOfField) {
	ASSERT_THROW(this->moveTo(location_id), fpmas::api::model::OutOfMobilityFieldException);
}

TEST_F(SpatialAgentTest, agentBehavior) {
	std::mt19937_64 engine;

	// Build NEW_MOVE layer
	std::vector<fpmas::model::AgentNode*> move_neighbors;
	std::vector<fpmas::model::AgentEdge*> move_neighbor_edges;
	for(unsigned int i = 0; i < 10; i++) {
		auto cell = new NiceMock<MockCell>;
		auto node = new MockDistributedNode<AgentPtr>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		move_neighbors.push_back(node);
		move_neighbor_edges.push_back(new MockDistributedEdge<AgentPtr>);
	}

	std::array<bool, 10> in_move_range;
	setUpRandomRange(
			engine, move_neighbors, move_neighbor_edges,
			in_move_range, this->mobility_range);

	// Build NEW_PERCEIVE layer
	std::vector<fpmas::model::AgentNode*> perceive_neighbors;
	std::vector<fpmas::model::AgentEdge*> perceive_neighbor_edges;
	for(unsigned int i = 0; i < 10; i++) {
		auto cell = new NiceMock<MockCell>;
		auto node = new MockDistributedNode<AgentPtr>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		perceive_neighbors.push_back(node);
		perceive_neighbor_edges.push_back(new MockDistributedEdge<AgentPtr>);
	}
	std::array<bool, 10> in_perceive_range;
	setUpRandomRange(
			engine, perceive_neighbors, perceive_neighbor_edges,
			in_perceive_range, this->perception_range);

	// Set up NEW_MOVE layer
	ON_CALL(agent_node, outNeighbors(EnvironmentLayers::NEW_MOVE))
		.WillByDefault(Return(move_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(EnvironmentLayers::NEW_MOVE))
		.WillByDefault(Return(move_neighbor_edges));

	// Set up NEW_PERCEIVE layer
	ON_CALL(agent_node, outNeighbors(EnvironmentLayers::NEW_PERCEIVE))
		.WillByDefault(Return(perceive_neighbors));
	ON_CALL(agent_node, getOutgoingEdges(EnvironmentLayers::NEW_PERCEIVE))
		.WillByDefault(Return(perceive_neighbor_edges));

	// Set up location
	ON_CALL(agent_node, getOutgoingEdges(EnvironmentLayers::LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::model::AgentEdge*> {&location_edge}));
	ON_CALL(agent_node, outNeighbors(EnvironmentLayers::LOCATION))
		.WillByDefault(Return(std::vector<fpmas::api::model::AgentNode*> {&location_node}));

	// MOVE expectations
	for(int i = 0; i < 10; i++) {
		if(in_move_range[i])
			EXPECT_CALL(mock_model, link(&agent, move_neighbors[i]->data().get(), EnvironmentLayers::MOVE));
		EXPECT_CALL(mock_model, unlink(move_neighbor_edges[i]));
	}
	this->handleNewMove();

	// PERCEIVE expectations
	for(int i = 0; i < 10; i++) {
		if(in_perceive_range[i])
			EXPECT_CALL(mock_model, link(&agent, perceive_neighbors[i]->data().get(), EnvironmentLayers::PERCEIVE));
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
	auto move_cell = new NiceMock<MockCell>;
	std::vector<fpmas::model::AgentNode*> move_neighbors;
	std::vector<fpmas::model::AgentEdge*> move_neighbor_edges;
	setUpDuplicates(
			move_cell, move_neighbors, move_neighbor_edges,
			EnvironmentLayers::NEW_MOVE, this->mobility_range);

	// Set up perceive layer with a duplicate node
	auto perceive_cell = new NiceMock<MockCell>;
	std::vector<fpmas::model::AgentNode*> perceive_neighbors;
	std::vector<fpmas::model::AgentEdge*> perceive_neighbor_edges;
	setUpDuplicates(
			perceive_cell, perceive_neighbors, perceive_neighbor_edges,
			EnvironmentLayers::NEW_PERCEIVE, this->perception_range);

	EXPECT_CALL(mock_model, link(&agent, move_cell, EnvironmentLayers::MOVE))
		.Times(1);
	for(auto edge : move_neighbor_edges)
		EXPECT_CALL(mock_model, unlink(edge));

	this->handleNewMove();

	EXPECT_CALL(mock_model, link(&agent, perceive_cell, EnvironmentLayers::PERCEIVE))
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
