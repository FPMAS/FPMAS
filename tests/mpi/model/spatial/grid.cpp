#include "fpmas/model/spatial/grid.h"
#include "fpmas/communication/communication.h"
#include "fpmas/model/spatial/grid_agent_mapping.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "fpmas/model/spatial/von_neumann.h"
#include "fpmas/model/spatial/moore.h"
#include "fpmas/model/spatial/von_neumann_grid.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "gmock/gmock.h"
#include "../test_agents.h"
#include "random/mock_random.h"
#include "model/mock_grid.h"

using namespace testing;
using namespace fpmas::model;

class GridBuilderTest : public Test {
	protected:
		void checkGridCells(
				fpmas::api::model::SpatialModel<fpmas::model::GridCell>& grid_model,
				int X, int Y
				) {
			fpmas::communication::TypedMpi<std::vector<fpmas::model::DiscretePoint>> mpi(
					fpmas::communication::WORLD);
			std::vector<fpmas::model::DiscretePoint> local_cell_coordinates;
			for(auto cell : grid_model.cells()) {
				fpmas::model::ReadGuard read(cell);
				local_cell_coordinates.push_back(
						dynamic_cast<fpmas::api::model::GridCell*>(cell)->location());
			}

			std::vector<std::vector<fpmas::api::model::DiscretePoint>> recv_coordinates
				= mpi.gather(local_cell_coordinates, 0);

			FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
				std::vector<fpmas::model::DiscretePoint> cell_coordinates;
				for(auto coordinates : recv_coordinates)
					cell_coordinates.insert(
							cell_coordinates.end(), coordinates.begin(), coordinates.end());
				ASSERT_THAT(cell_coordinates, SizeIs(X * Y));

				std::vector<fpmas::model::DiscretePoint> expected_coordinates;
				for(fpmas::model::DiscreteCoordinate i = 0; i < X; i++)
					for(fpmas::model::DiscreteCoordinate j = 0; j < Y; j++)
						expected_coordinates.push_back({i, j});
				ASSERT_THAT(cell_coordinates, UnorderedElementsAreArray(expected_coordinates));
			}

		}

};

class VonNeumannGridBuilderTest : public GridBuilderTest {
	protected:
		void checkGridStructure(
				fpmas::api::model::SpatialModel<fpmas::model::GridCell>& grid_model,
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) {
			checkGridCells(grid_model, X, Y);

			for(auto grid_cell : cells) {
				fpmas::model::ReadGuard read(grid_cell);
				auto location = grid_cell->location();
				auto cell_successors = grid_cell->successors();
				std::vector<fpmas::model::DiscretePoint> neighbor_points;
				for(auto cell : cell_successors) {
					fpmas::model::ReadGuard read(cell);
					neighbor_points.push_back(
							dynamic_cast<fpmas::api::model::GridCell*>(cell)->location());
				}

				// Any cell in the center of the grid
				if(location.x > 0 && location.x < X-1
						&& location.y > 0 && location.y < Y-1) {
					ASSERT_THAT(neighbor_points, UnorderedElementsAre(
								fpmas::model::DiscretePoint(location.x, location.y-1),
								fpmas::model::DiscretePoint(location.x, location.y+1),
								fpmas::model::DiscretePoint(location.x-1, location.y),
								fpmas::model::DiscretePoint(location.x+1, location.y)
								));
				}

				if(location.x == 0) {
					if(location.y == 0) {
						// Bottom left corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, 1),
									fpmas::model::DiscretePoint(1, 0)));
					} else if(location.y == Y-1) {
						// Up left corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, Y-2),
									fpmas::model::DiscretePoint(1, Y-1)));
					} else {
						// Left border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, location.y-1),
									fpmas::model::DiscretePoint(0, location.y+1),
									fpmas::model::DiscretePoint(1, location.y)));
					}
				} else if (location.x == X-1) {
					if(location.y == 0) {
						// Bottom right corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-2, 0),
									fpmas::model::DiscretePoint(X-1, 1)));
					} else if(location.y == Y-1) {
						// Up right corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, Y-2),
									fpmas::model::DiscretePoint(X-2, Y-1)));
					} else {
						// Right border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, location.y-1),
									fpmas::model::DiscretePoint(X-1, location.y+1),
									fpmas::model::DiscretePoint(X-2, location.y)));
					}
				} else {
					if(location.y == 0) {
						// Bottom border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(location.x-1, 0),
									fpmas::model::DiscretePoint(location.x+1, 0),
									fpmas::model::DiscretePoint(location.x, 1)));
					} else if (location.y == Y-1) {
						// Top border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(location.x-1, Y-1),
									fpmas::model::DiscretePoint(location.x+1, Y-1),
									fpmas::model::DiscretePoint(location.x, Y-2)));
					}
				}
			}

			grid_model.graph().synchronize();
		}
};

TEST_F(VonNeumannGridBuilderTest, trivial) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	VonNeumannGridBuilder<> null(0, 0);

	null.build(grid_model);
	ASSERT_THAT(grid_model.cells(), IsEmpty());
	ASSERT_EQ(null.height(), 0);
	ASSERT_EQ(null.width(), 0);
}

TEST_F(VonNeumannGridBuilderTest, ghost_mode_build_height_sup_width) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, ghost_mode_build_width_sup_height) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int Y = fpmas::communication::WORLD.getSize() * 10;
	int X = 2*Y + 1; // +1 so that there is a "remainder" when columns are assigned to each process
	VonNeumannGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, hard_sync_mode_build_height_sup_width) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	VonNeumannGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, hard_sync_mode_build_width_sup_height) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int Y = fpmas::communication::WORLD.getSize() * 10;
	int X = 2*Y + 1; // +1 so that there is a "remainder" when columns are assigned to each process
	VonNeumannGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, build_with_groups) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	IdleBehavior behavior;
	auto& g1 = grid_model.buildGroup(0, behavior);
	auto& g2 = grid_model.buildGroup(1, behavior);

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model, {g1, g2});

	checkGridStructure(grid_model, cells, X, Y);

	for(auto cell : cells) {
		ASSERT_THAT(cell->groups(), Contains(&g1));
		ASSERT_THAT(cell->groups(), Contains(&g2));
		ASSERT_THAT(g1.localAgents(), Contains(cell));
		ASSERT_THAT(g2.localAgents(), Contains(cell));
	}
}

class MooreGridBuilderTest : public GridBuilderTest {
	protected:
		void checkGridStructure(
				fpmas::api::model::SpatialModel<fpmas::model::GridCell>& grid_model,
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) {
			checkGridCells(grid_model, X, Y);

			for(auto grid_cell : cells) {
				fpmas::model::ReadGuard read(grid_cell);
				auto location = grid_cell->location();
				auto cell_successors = grid_cell->successors();
				std::vector<fpmas::model::DiscretePoint> neighbor_points;
				for(auto cell : cell_successors) {
					fpmas::model::ReadGuard read(cell);
					neighbor_points.push_back(
							dynamic_cast<fpmas::api::model::GridCell*>(cell)->location());
				}

				// Any cell in the center of the grid
				if(location.x > 0 && location.x < X-1
						&& location.y > 0 && location.y < Y-1) {
					ASSERT_THAT(neighbor_points, UnorderedElementsAre(
								// Cells in the VonNeumann range
								fpmas::model::DiscretePoint(location.x, location.y-1),
								fpmas::model::DiscretePoint(location.x, location.y+1),
								fpmas::model::DiscretePoint(location.x-1, location.y),
								fpmas::model::DiscretePoint(location.x+1, location.y),
								// Additionnal corners
								fpmas::model::DiscretePoint(location.x+1, location.y+1),
								fpmas::model::DiscretePoint(location.x+1, location.y-1),
								fpmas::model::DiscretePoint(location.x-1, location.y+1),
								fpmas::model::DiscretePoint(location.x-1, location.y-1)
								));
				}

				if(location.x == 0) {
					if(location.y == 0) {
						// Bottom left corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, 1),
									fpmas::model::DiscretePoint(1, 0),
									fpmas::model::DiscretePoint(1, 1)
									));
					} else if(location.y == Y-1) {
						// Up left corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, Y-2),
									fpmas::model::DiscretePoint(1, Y-1),
									fpmas::model::DiscretePoint(1, Y-2)
									));
					} else {
						// Left border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, location.y-1),
									fpmas::model::DiscretePoint(0, location.y+1),
									fpmas::model::DiscretePoint(1, location.y),
									fpmas::model::DiscretePoint(1, location.y-1),
									fpmas::model::DiscretePoint(1, location.y+1)
									));
					}
				} else if (location.x == X-1) {
					if(location.y == 0) {
						// Bottom right corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-2, 0),
									fpmas::model::DiscretePoint(X-1, 1),
									fpmas::model::DiscretePoint(X-2, 1)
									));
					} else if(location.y == Y-1) {
						// Up right corner
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, Y-2),
									fpmas::model::DiscretePoint(X-2, Y-1),
									fpmas::model::DiscretePoint(X-2, Y-2)
									));
					} else {
						// Right border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, location.y-1),
									fpmas::model::DiscretePoint(X-1, location.y+1),
									fpmas::model::DiscretePoint(X-2, location.y),
									fpmas::model::DiscretePoint(X-2, location.y-1),
									fpmas::model::DiscretePoint(X-2, location.y+1)
									));
					}
				} else {
					if(location.y == 0) {
						// Bottom border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(location.x-1, 0),
									fpmas::model::DiscretePoint(location.x+1, 0),
									fpmas::model::DiscretePoint(location.x, 1),
									fpmas::model::DiscretePoint(location.x-1, 1),
									fpmas::model::DiscretePoint(location.x+1, 1)
									));
					} else if (location.y == Y-1) {
						// Top border
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(location.x-1, Y-1),
									fpmas::model::DiscretePoint(location.x+1, Y-1),
									fpmas::model::DiscretePoint(location.x, Y-2),
									fpmas::model::DiscretePoint(location.x-1, Y-2),
									fpmas::model::DiscretePoint(location.x+1, Y-2)
									));
					}
				}
			}

			grid_model.graph().synchronize();
		}
};

TEST_F(MooreGridBuilderTest, trivial) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	MooreGridBuilder<> null(0, 0);

	null.build(grid_model);
	ASSERT_THAT(grid_model.cells(), IsEmpty());
	ASSERT_EQ(null.height(), 0);
	ASSERT_EQ(null.width(), 0);
}

TEST_F(MooreGridBuilderTest, ghost_mode_build_height_sup_width) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, ghost_mode_build_width_sup_height) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int Y = fpmas::communication::WORLD.getSize() * 10;
	int X = 2*Y + 1; // +1 so that there is a "remainder" when columns are assigned to each process
	MooreGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, hard_sync_mode_build_height_sup_width) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	MooreGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, hard_sync_mode_build_width_sup_height) {
	GridModel<fpmas::synchro::HardSyncMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int Y = fpmas::communication::WORLD.getSize() * 10;
	int X = 2*Y + 1; // +1 so that there is a "remainder" when columns are assigned to each process
	MooreGridBuilder<> grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, build_with_groups) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	IdleBehavior behavior;
	auto& g1 = grid_model.buildGroup(0, behavior);
	auto& g2 = grid_model.buildGroup(1, behavior);

	int X = fpmas::communication::WORLD.getSize() * 10;
	int Y = 2*X + 1; // +1 so that there is a "remainder" when lines are assigned to each process
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model, {g1, g2});

	checkGridStructure(grid_model, cells, X, Y);

	for(auto cell : cells) {
		ASSERT_THAT(cell->groups(), Contains(&g1));
		ASSERT_THAT(cell->groups(), Contains(&g2));
		ASSERT_THAT(g1.localAgents(), Contains(cell));
		ASSERT_THAT(g2.localAgents(), Contains(cell));
	}
}

class GridAgentBuilderTest : public Test {
	protected:
		static const std::size_t NUM_AGENT_BY_PROC = 20;
		fpmas::model::GridModel<fpmas::synchro::GhostMode> model;
		fpmas::model::GridAgentBuilder<fpmas::model::GridCell> agent_builder;

		void SetUp() {
			auto grid_width = 10*model.getMpiCommunicator().getSize();
			auto grid_height = 20;
			auto& agent_group = model.buildGroup(1);
			fpmas::model::VonNeumannGridBuilder<fpmas::model::GridCell> grid_builder(
					grid_width, grid_height);
			grid_builder.build(model);

			fpmas::model::UniformGridAgentMapping agent_mapping(
					grid_width, grid_height, NUM_AGENT_BY_PROC*model.getMpiCommunicator().getSize());
			fpmas::model::DefaultSpatialAgentFactory<TestGridAgent> agent_factory;

			agent_builder.build(
					model, {agent_group}, agent_factory, agent_mapping);
		}
};

TEST_F(GridAgentBuilderTest, build) {
	std::vector<fpmas::random::mt19937_64> random_generators;
	for(auto agent : model.getGroup(1).localAgents())
		random_generators.push_back(((TestGridAgent*) agent)->rd());

	fpmas::communication::TypedMpi<decltype(random_generators)> mpi(
				model.getMpiCommunicator());
	random_generators
		= fpmas::communication::all_reduce(mpi, random_generators, fpmas::utils::Concat());
	for(std::size_t i = 0; i < random_generators.size()-1; i++) {
		for(std::size_t j = i+1; j < random_generators.size(); j++) {
			ASSERT_NE(random_generators[i], random_generators[j]);
		}
	}
}

TEST_F(GridAgentBuilderTest, init_sample) {
	std::size_t num_call = 0;
	std::set<fpmas::model::DistributedId> agent_ids;

	std::size_t sample_size = (NUM_AGENT_BY_PROC/2)*model.getMpiCommunicator().getSize();
	agent_builder.init_sample(
			sample_size,
			[&num_call, &agent_ids] (fpmas::api::model::GridAgent<fpmas::model::GridCell>* agent) {
				num_call++;
				agent_ids.insert(agent->node()->getId());
			});

	fpmas::communication::TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	num_call = fpmas::communication::all_reduce(int_mpi, num_call);

	ASSERT_EQ(num_call, sample_size);

	fpmas::communication::TypedMpi<decltype(agent_ids)> id_mpi(model.getMpiCommunicator());

	agent_ids = fpmas::communication::all_reduce(id_mpi, agent_ids, fpmas::utils::Concat());
	ASSERT_THAT(agent_ids, SizeIs(sample_size));
}
