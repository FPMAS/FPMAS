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
		fpmas::model::Behavior<TestGridAgent> behavior {&TestGridAgent::move};

		void SetUp() override {
			/*
			 * Builds and distribute a grid
			 */
			fpmas::model::VonNeumannGridBuilder<> grid_builder(
					model.getMpiCommunicator().getSize() * 2, 2
					);
			grid_builder.build(model);

			fpmas::graph::RandomLoadBalancing<fpmas::api::model::AgentPtr> random_lb(
					model.getMpiCommunicator()
					);
			// Initial random partitioning
			//fpmas::api::graph::PartitionMap partition 
				//= random_lb.balance(model.graph().getLocationManager().getLocalNodes());
			//model.graph().distribute(partition);

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
			auto partition = random_lb.balance(model.graph().getLocationManager().getLocalNodes());
			model.graph().distribute(partition);

		}

		void check_agent_cells() {
			auto& agent_group = model.getGroup(0);
			for(auto agent : agent_group.localAgents())
				ASSERT_THAT(
						dynamic_cast<fpmas::api::model::GridAgent<fpmas::model::GridCell>*>(
							agent
							)->locationCell()->node()->state(),
						fpmas::api::graph::LOCAL
						);
			for(auto cell : model.cells())
				ASSERT_FLOAT_EQ(cell->node()->getWeight(), 1.f);

		}
};

TEST_F(CellLoadBalancingTest, dynamic) {
	// Cell partitioning
	fpmas::model::CellLoadBalancing cell_lb(
			model.getMpiCommunicator(), zoltan_lb
			);
	auto partition = cell_lb.balance(
			model.graph().getLocationManager().getLocalNodes(),
			fpmas::api::graph::PARTITION
			);
	model.graph().distribute(partition);

	auto& agent_group = model.getGroup(0);
	check_agent_cells();

	for(int i = 0; i < 10; i++) {
		// Agents move to adjacent cells
		model.runtime().execute(agent_group.jobs());

		// Grid repartitioning
		partition = cell_lb.balance(
				model.graph().getLocationManager().getLocalNodes(),
				fpmas::api::graph::REPARTITION
				);
		model.graph().distribute(partition);
		check_agent_cells();
	}
}

TEST_F(CellLoadBalancingTest, static) {
	fpmas::model::StaticCellLoadBalancing cell_lb(
			model.getMpiCommunicator(), zoltan_lb
			);

	auto partition = cell_lb.balance(
			model.graph().getLocationManager().getLocalNodes(),
			fpmas::api::graph::PARTITION
			);
	model.graph().distribute(partition);

	auto& agent_group = model.getGroup(0);
	check_agent_cells();
	std::set<fpmas::api::graph::DistributedId> init_local_cells;
	for(auto cell : model.cells())
		init_local_cells.insert(cell->node()->getId());

	for(int i = 0; i < 10; i++) {
		// Agents move to adjacent cells
		model.runtime().execute(agent_group.jobs());

		// Grid repartitioning
		partition = cell_lb.balance(
				model.graph().getLocationManager().getLocalNodes(),
				fpmas::api::graph::REPARTITION
				);
		model.graph().distribute(partition);

		std::set<fpmas::api::graph::DistributedId> local_cells;
		for(auto cell : model.cells())
			local_cells.insert(cell->node()->getId());
		check_agent_cells();
		ASSERT_THAT(local_cells, ElementsAreArray(init_local_cells));
	}
}
