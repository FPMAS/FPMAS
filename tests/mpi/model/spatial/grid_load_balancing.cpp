#include "gmock/gmock.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/model/spatial/grid_load_balancing.h"
#include "fpmas/graph/random_load_balancing.h"
#include "../../../mocks/model/mock_spatial_model.h"
#include "../test_agents.h"

using namespace testing;

class GridLoadBalancingTest : public Test {
	protected:
		fpmas::model::GridModel<fpmas::synchro::GhostMode> model;
};

TEST_F(GridLoadBalancingTest, width_sup_height) {
	fpmas::model::VonNeumannGridBuilder<> grid_builder(
			model.getMpiCommunicator().getSize() * 2, 2
			);
	grid_builder.build(model);

	fpmas::model::GridLoadBalancing grid_lb(
			grid_builder.width(), grid_builder.height(), model.getMpiCommunicator()
			);
	fpmas::graph::RandomLoadBalancing<fpmas::api::model::AgentPtr> random_lb(
			model.getMpiCommunicator()
			);
	// Initial random partitioning
	fpmas::api::graph::PartitionMap partition 
		= random_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	partition = grid_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	// Clears useless DISTANT nodes
	model.graph().synchronize();

	ASSERT_EQ(model.graph().getLocationManager().getLocalNodes().size(), 4);

	fpmas::communication::TypedMpi<std::size_t> mpi(model.getMpiCommunicator());
	std::vector<std::size_t> distant_node_counts =
		mpi.allGather(model.graph().getLocationManager().getDistantNodes().size());

	std::vector<std::size_t> distant_node_counts_expectations;
	if(model.getMpiCommunicator().getSize() > 1) {
		distant_node_counts_expectations.push_back(2);
		distant_node_counts_expectations.push_back(2);
	}
	for(int i = 2; i < model.getMpiCommunicator().getSize(); i++)
		distant_node_counts_expectations.push_back(4);
	ASSERT_THAT(
			distant_node_counts,
			UnorderedElementsAreArray(distant_node_counts_expectations)
			);
}

TEST_F(GridLoadBalancingTest, height_sup_width) {
	fpmas::model::VonNeumannGridBuilder<> grid_builder(
			2, model.getMpiCommunicator().getSize() * 2
			);
	grid_builder.build(model);

	fpmas::model::GridLoadBalancing grid_lb(
			grid_builder.width(), grid_builder.height(), model.getMpiCommunicator()
			);
	fpmas::graph::RandomLoadBalancing<fpmas::api::model::AgentPtr> random_lb(
			model.getMpiCommunicator()
			);
	// Initial random partitioning
	fpmas::api::graph::PartitionMap partition 
		= random_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	partition = grid_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	// Clears useless DISTANT nodes
	model.graph().synchronize();

	ASSERT_EQ(model.graph().getLocationManager().getLocalNodes().size(), 4);

	fpmas::communication::TypedMpi<std::size_t> mpi(model.getMpiCommunicator());
	std::vector<std::size_t> distant_node_counts =
		mpi.allGather(model.graph().getLocationManager().getDistantNodes().size());

	std::vector<std::size_t> distant_node_counts_expectations;
	if(model.getMpiCommunicator().getSize() > 1) {
		distant_node_counts_expectations.push_back(2);
		distant_node_counts_expectations.push_back(2);
	}
	for(int i = 2; i < model.getMpiCommunicator().getSize(); i++)
		distant_node_counts_expectations.push_back(4);
	ASSERT_THAT(
			distant_node_counts,
			UnorderedElementsAreArray(distant_node_counts_expectations)
			);
}

TEST_F(GridLoadBalancingTest, grid_agents) {
	/*
	 * Builds and distributed a grid
	 */
	fpmas::model::VonNeumannGridBuilder<> grid_builder(
			model.getMpiCommunicator().getSize() * 2, 2
			);
	grid_builder.build(model);

	fpmas::model::GridLoadBalancing grid_lb(
			grid_builder.width(), grid_builder.height(), model.getMpiCommunicator()
			);

	fpmas::api::graph::PartitionMap partition
		= grid_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	// Clears useless DISTANT nodes
	model.graph().synchronize();

	fpmas::model::IdleBehavior behavior;
	auto& move_group = model.buildMoveGroup(0, behavior);

	std::vector<TestGridAgent*> agents;
	for(auto cell : model.graph().getLocationManager().getDistantNodes()) {
		auto agent = new TestGridAgent;
		move_group.add(agent);
		agent->initLocation(
				dynamic_cast<fpmas::api::model::GridCell*>(cell.second->data().get())
				);
	}
	model.runtime().execute(move_group.jobs());

	partition = grid_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	for(auto agent : move_group.localAgents())
		ASSERT_THAT(
				dynamic_cast<fpmas::api::model::GridAgent<fpmas::api::model::GridCell>*>(
					agent
					)->locationCell()->node()->state(),
				fpmas::api::graph::LOCAL
				);
}
