#include "../../../mocks/model/mock_spatial_model.h"
#include "../../../mocks/communication/mock_communication.h"
#include "../../../mocks/synchro/mock_mutex.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "test_agents.h"


using fpmas::api::model::SpatialModelLayers;

using namespace testing;
using namespace fpmas::model;

class BaseCell : public Cell<BaseCell> {
};

/*
 * MockCellBase inherits from fpmas::model::CellBase, so that methods defined
 * in fpmas::api::model::Cell are implemented, but other
 * fpmas::api::model::Agent methods are mocked.
 * See mocks/model/mock_environment.h.
 */

class CellBaseTest : public Test {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;
		BaseCell cell;

		DistributedId agent_id {7, 4};
		// NiceMock is only applied to MockAgentBase (googletest limitation)
		MockSpatialAgent<DefaultCell>* mock_spatial_agent
			= new NiceMock<MockSpatialAgent<DefaultCell>>;
		NiceMock<MockCell<>> current_location;
		MockAgentNode<NiceMock> agent_node {
			agent_id, AgentPtr(mock_spatial_agent)
		};
		MockAgentEdge<NiceMock> agent_edge;

		MockAgentNode<NiceMock> cell_node;

		BaseCell* cell_neighbor_1 = new BaseCell;
		MockAgentNode<NiceMock> cell_neighbor_1_node {
			agent_id, AgentPtr(cell_neighbor_1)};
		MockAgentEdge<NiceMock> cell_neighbor_1_edge;

		BaseCell* cell_neighbor_2 = new BaseCell;
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

			cell.setModel(&mock_model);
			cell.setNode(&cell_node);

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

/**
 * IMPORTANT NOTE
 *
 * The handleNewLocation, handleMove and handlePerceive methods are
 * not tested here, but their integration are directly tested in
 * tests/mpi/model/spatial/spatial_model.cpp
 *
 * Optimization of those methods actually revealed that local unit tests were
 * too constrained: it is more efficient to directly test the
 * DistributedMoveAlgorithm integration.
 *
 * The same consideration might soon be applied to handleMove and
 * handlePerceive, since those tests are not really relevant, for the same
 * reasons.
 */

/*
 *TEST_F(CellBaseTest, handle_move) {
 *}
 */

/*
 *TEST_F(CellBaseTest, handle_perceive) {
 *}
 */

TEST_F(CellBaseTest, update_perceptions) {
	MockAgentGraph<> mock_graph;
	ON_CALL(mock_model, graph())
		.WillByDefault(ReturnRef(mock_graph));
	EXPECT_CALL(mock_model, graph())
		.Times(AnyNumber());

	DistributedId perceived_id {5, 2};
	MockSpatialAgent<DefaultCell>* perceived_agent
		= new NiceMock<MockSpatialAgent<DefaultCell>>;

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
	ON_CALL(cell_node, inNeighbors(SpatialModelLayers::PERCEIVE))
		.WillByDefault(Return(std::vector<AgentNode*>({&agent_node})));

	ON_CALL(cell_node, getIncomingEdges(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentEdge*>(
						{&agent_edge, &perceived_agent_edge}
						)));
	ON_CALL(cell_node, inNeighbors(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentNode*>(
						{&agent_node, &perceived_agent_node}
						)));


	EXPECT_CALL(mock_graph, link(
				&agent_node, &perceived_agent_node, SpatialModelLayers::PERCEPTION));

	cell.updatePerceptions(mock_group);
}

/*
 * Inheriting from GridAgentType is an hacky trick to get access to protected
 * GridAgentType members (such as moveTo())
 */
template<typename AgentType>
class BaseSpatialAgentTest : public ::testing::Test, protected AgentType {
	protected:
		typedef fpmas::api::model::Cell DefaultCell;

		MockMutex<AgentPtr> mock_location_mutex;
		StrictMock<MockModel> mock_model;
		AgentType& agent;
		MockAgentNode<NiceMock> agent_node {{2, 37}, this};

		fpmas::graph::DistributedId location_id {12, 67};
		NiceMock<MockCell<>> location_cell;
		MockAgentEdge<NiceMock> location_edge;
		MockAgentNode<NiceMock> location_node {location_id, &location_cell};

		BaseSpatialAgentTest() : agent(*this) {}


		void SetUp() override {
			ON_CALL(location_node, mutex())
				.WillByDefault(Return(&mock_location_mutex));
			ON_CALL(Const(location_node), mutex())
				.WillByDefault(Return(&mock_location_mutex));
			ON_CALL(mock_location_mutex, read)
				.WillByDefault(ReturnRef(location_node.data()));
			// By default, no expectation.
			// Can be overriden with explicit EXPECT_CALLS
			EXPECT_CALL(mock_location_mutex, read).Times(AnyNumber());
			EXPECT_CALL(mock_location_mutex, releaseRead).Times(AnyNumber());

			this->setNode(&agent_node);
			this->setModel(&mock_model);

			location_edge.setSourceNode(&agent_node);
			location_edge.setTargetNode(&location_node);

			ON_CALL(location_cell, node())
				.WillByDefault(Return(&location_node));
			ON_CALL(Const(location_cell), node())
				.WillByDefault(Return(&location_node));
		}

		void TearDown() override {
			agent_node.data().release();
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

		void setUpDuplicates(
				MockCell<>* cell,
				std::vector<AgentNode*>& nodes,
				std::vector<AgentEdge*>& edges,
				fpmas::graph::LayerId layer,
				MockRange<DefaultCell>& range) {

			auto node = new MockAgentNode<NiceMock>({0, 1}, cell);
			ON_CALL(*cell, node())
				.WillByDefault(Return(node));
			ON_CALL(Const(*cell), node())
				.WillByDefault(Return(node));
			auto mutex = new NiceMock<MockMutex<AgentPtr>>;
			ON_CALL(*mutex, read)
				.WillByDefault(ReturnRef(node->data()));
			// The mutex will be automatically deleted with `node`
			node->setMutex(mutex);

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

		template<typename PackType>
			void Serialize(PackType& pack) {
				pack = 
					fpmas::api::utils::PtrWrapper<typename AgentType::JsonBase>(&agent);
			}

		template<typename PackType>
			fpmas::api::utils::PtrWrapper<typename AgentType::JsonBase> Unserialize(const PackType& pack) {
				return pack
					.template get<fpmas::api::utils::PtrWrapper<typename AgentType::JsonBase>>();
			}

};

typedef BaseSpatialAgentTest<model::test::SpatialAgent> SpatialAgentTest;

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
	std::vector<MockMutex<AgentPtr>*> move_mutexes;
	for(FPMAS_ID_TYPE i = 0; i < 10; i++) {
		auto cell = new NiceMock<MockCell<>>;
		auto node = new MockAgentNode<NiceMock>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		ON_CALL(Const(*cell), node())
			.WillByDefault(Return(node));
		move_mutexes.push_back(new MockMutex<AgentPtr>);
		move_neighbors.push_back(node);
		move_neighbor_edges.push_back(new MockAgentEdge<NiceMock>);
		ON_CALL(*node, mutex())
			.WillByDefault(Return(move_mutexes.back()));
		ON_CALL(Const(*node), mutex())
			.WillByDefault(Return(move_mutexes.back()));
	}

	std::array<bool, 10> in_move_range;
	setUpRandomRange(
			engine, move_neighbors, move_neighbor_edges,
			in_move_range, this->mobility_range);

	// Build NEW_PERCEIVE layer
	std::vector<AgentNode*> perceive_neighbors;
	std::vector<AgentEdge*> perceive_neighbor_edges;
	std::vector<MockMutex<AgentPtr>*> perceive_mutexes;
	for(FPMAS_ID_TYPE i = 0; i < 10; i++) {
		auto cell = new NiceMock<MockCell<>>;
		auto node = new MockAgentNode<NiceMock>({0, i}, cell);
		ON_CALL(*cell, node())
			.WillByDefault(Return(node));
		ON_CALL(Const(*cell), node())
			.WillByDefault(Return(node));
		perceive_mutexes.push_back(new MockMutex<AgentPtr>);
		perceive_neighbors.push_back(node);
		perceive_neighbor_edges.push_back(new MockAgentEdge<NiceMock>);
		ON_CALL(*node, mutex())
			.WillByDefault(Return(perceive_mutexes.back()));
		ON_CALL(Const(*node), mutex())
			.WillByDefault(Return(perceive_mutexes.back()));
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
	EXPECT_CALL(mock_location_mutex, read);
	EXPECT_CALL(mock_location_mutex, releaseRead);
	for(std::size_t i = 0; i < move_neighbors.size(); i++) {
		EXPECT_CALL(*move_mutexes[i], read)
			.WillOnce(ReturnRef(move_neighbors[i]->data()));
		EXPECT_CALL(*move_mutexes[i], releaseRead);
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
	EXPECT_CALL(mock_location_mutex, read);
	EXPECT_CALL(mock_location_mutex, releaseRead);
	for(std::size_t i = 0; i < perceive_neighbors.size(); i++) {
		EXPECT_CALL(*perceive_mutexes[i], read)
			.WillOnce(ReturnRef(perceive_neighbors[i]->data()));
		EXPECT_CALL(*perceive_mutexes[i], releaseRead);
	}
	this->handleNewPerceive();

	for(auto node : move_neighbors)
		delete node;
	for(auto edge : move_neighbor_edges)
		delete edge;
	for(auto mutex : move_mutexes)
		delete mutex;

	for(auto node : perceive_neighbors)
		delete node;
	for(auto edge : perceive_neighbor_edges)
		delete edge;
	for(auto mutex : perceive_mutexes)
		delete mutex;
}

/*
 * While exploring the graph, because of cycles, the same node might be added
 * several times in the NEW_MOVE or NEW_PERCEIVE layer at the same iteration.
 * In this case, the corresponding node must be linked **exactly once** on the
 * MOVE or PERCEIVE layer, and **all** corresponding edges must be unlinked.
 */
TEST_F(SpatialAgentTest, agentBehaviorWithDuplicates) {
	EXPECT_CALL(mock_location_mutex, read).Times(AnyNumber());
	EXPECT_CALL(mock_location_mutex, releaseRead).Times(AnyNumber());

	// Set up location
	ON_CALL(agent_node, getOutgoingEdges(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentEdge*> {
					&location_edge}));
	ON_CALL(agent_node, outNeighbors(SpatialModelLayers::LOCATION))
		.WillByDefault(Return(std::vector<AgentNode*> {
					&location_node}));

	// Set up move layer with a duplicate node
	auto move_cell = new NiceMock<MockCell<>>;
	std::vector<AgentNode*> move_neighbors;
	std::vector<AgentEdge*> move_neighbor_edges;
	setUpDuplicates(
			move_cell, move_neighbors, move_neighbor_edges,
			SpatialModelLayers::NEW_MOVE, this->mobility_range);

	// Set up perceive layer with a duplicate node
	auto perceive_cell = new NiceMock<MockCell<>>;
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

#define SPATIAL_AGENT_SERIAL_TEST_SUITE(AGENT_TYPE)\
	TEST_F(AGENT_TYPE##Test, json) {\
		EXPECT_CALL(mock_model, link)\
		.Times(AnyNumber());\
		this->initLocation(&location_cell);\
		\
		nlohmann::json j;\
		Serialize(j);\
		\
		auto ptr = Unserialize(j);\
		\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		ASSERT_EQ(static_cast<AGENT_TYPE*>(ptr.get())->locationId(), location_id);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*static_cast<AGENT_TYPE*>(ptr.get()), agent);\
		delete ptr.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, light_json) {\
		EXPECT_CALL(mock_model, link)\
		.Times(AnyNumber());\
		this->initLocation(&location_cell);\
		\
		nlohmann::json j;\
		Serialize(j);\
		fpmas::io::json::light_json light_json;\
		Serialize(light_json);\
		\
		ASSERT_LT(light_json.dump().size(), j.dump().size());\
		\
		auto ptr = Unserialize(j);\
		\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		delete ptr.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, object_pack) {\
		EXPECT_CALL(mock_model, link)\
		.Times(AnyNumber());\
		this->initLocation(&location_cell);\
		\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		\
		auto ptr = Unserialize(pack);\
		\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		ASSERT_EQ(static_cast<AGENT_TYPE*>(ptr.get())->locationId(), location_id);\
		/* Checks optional cell data equality */\
		ASSERT_EQ(*static_cast<AGENT_TYPE*>(ptr.get()), agent);\
		delete ptr.get();\
	}\
	\
	TEST_F(AGENT_TYPE##Test, light_object_pack) {\
		EXPECT_CALL(mock_model, link)\
		.Times(AnyNumber());\
		this->initLocation(&location_cell);\
		\
		fpmas::io::datapack::ObjectPack pack;\
		Serialize(pack);\
		fpmas::io::datapack::LightObjectPack light_object_pack;\
		Serialize(light_object_pack);\
		\
		ASSERT_LT(light_object_pack.data().size, pack.data().size);\
		\
		auto ptr = Unserialize(pack);\
		\
		ASSERT_THAT(ptr.get(), WhenDynamicCastTo<AGENT_TYPE*>(NotNull()));\
		delete ptr.get();\
	}\

typedef BaseSpatialAgentTest<model::test::SpatialAgentWithData> SpatialAgentWithDataTest;

namespace {
	using model::test::SpatialAgent;

	/*
	 * Serialization tests for an fpmas::model::SpatialAgent extension, serialized
	 * using FPMAS_DEFAULT_JSON() and FPMAS_DEFAULT_DATAPACK().
	 */
	SPATIAL_AGENT_SERIAL_TEST_SUITE(SpatialAgent);

	using model::test::SpatialAgentWithData;

	/*
	 * Serialization tests for an fpmas::model::SpatialAgent
	 * extension with custom serialization rules.
	 */
	SPATIAL_AGENT_SERIAL_TEST_SUITE(SpatialAgentWithData);
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
