#include "gmock/gmock.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/model/spatial/cell_load_balancing.h"
#include "fpmas/graph/random_load_balancing.h"
#include "../test_agents.h"

using namespace testing;

class CellLoadBalancingTest : public Test {
	protected:
		fpmas::model::GridModel<fpmas::synchro::GhostMode> model;
		fpmas::model::ZoltanLoadBalancing zoltan_lb {fpmas::communication::WORLD};
};

TEST_F(CellLoadBalancingTest, cell_agents) {
	/*
	 * Builds and distributed a grid
	 */
	fpmas::model::VonNeumannGridBuilder<> grid_builder(
			model.getMpiCommunicator().getSize() * 2, 2
			);
	grid_builder.build(model);

	fpmas::model::CellLoadBalancing cell_lb(
			model.getMpiCommunicator(), zoltan_lb
			);
	fpmas::graph::RandomLoadBalancing<fpmas::api::model::AgentPtr> random_lb(
			model.getMpiCommunicator()
			);
	// Initial random partitioning
	fpmas::api::graph::PartitionMap partition 
		= random_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	fpmas::model::Behavior<TestGridAgent> behavior(&TestGridAgent::move);
	auto& move_group = model.buildMoveGroup(0, behavior);

	std::vector<TestGridAgent*> agents;
	for(auto cell : model.graph().getLocationManager().getDistantNodes()) {
		auto agent = new TestGridAgent;
		move_group.add(agent);
		agent->initLocation(
				dynamic_cast<fpmas::model::GridCell*>(cell.second->data().get())
				);
	}
	model.runtime().execute(move_group.distributedMoveAlgorithm().jobs());
	// Initial random partitioning (with agents)
	partition = random_lb.balance(model.graph().getLocationManager().getLocalNodes());
	model.graph().distribute(partition);

	// Cell partitioning
	partition = cell_lb.balance(
			model.graph().getLocationManager().getLocalNodes(),
			fpmas::api::graph::PARTITION
			);
	model.graph().distribute(partition);

	for(auto agent : move_group.localAgents())
		ASSERT_THAT(
				dynamic_cast<fpmas::api::model::GridAgent<fpmas::model::GridCell>*>(
					agent
					)->locationCell()->node()->state(),
				fpmas::api::graph::LOCAL
				);
	for(auto cell : model.cells())
		ASSERT_FLOAT_EQ(cell->node()->getWeight(), 1.f);

	// Agents move to adjacent cells
	model.runtime().execute(move_group.jobs());

	// Grid repartitioning
	partition = cell_lb.balance(
			model.graph().getLocationManager().getLocalNodes(),
			fpmas::api::graph::REPARTITION
			);
	model.graph().distribute(partition);

	for(auto agent : move_group.localAgents())
		ASSERT_THAT(
				dynamic_cast<fpmas::api::model::GridAgent<fpmas::model::GridCell>*>(
					agent
					)->locationCell()->node()->state(),
				fpmas::api::graph::LOCAL
				);
	for(auto cell : model.cells())
		ASSERT_FLOAT_EQ(cell->node()->getWeight(), 1.f);
}
