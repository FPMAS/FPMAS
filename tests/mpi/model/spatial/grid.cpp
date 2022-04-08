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
#include "../../../mocks/random/mock_random.h"
#include "../../../mocks/model/mock_grid.h"

using namespace testing;
using namespace fpmas::model;

namespace fpmas { namespace random {
	bool operator!=(const Generator<std::random_device>&, const Generator<std::random_device>&) {
		return true;
	}
}}

class GridBuilderTestBase : public Test {
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

		/* Trivial cases used both for Moore and VonNeumann grids */

		void checkVerticalLine(std::vector<fpmas::model::GridCell*> cells, int Y) {

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
				if(location.y == 0) {
					ASSERT_THAT(neighbor_points, ElementsAre(
								fpmas::model::DiscretePoint(0, location.y + 1)
								));
				} else if (location.y == Y-1) {
					ASSERT_THAT(neighbor_points, ElementsAre(
								fpmas::model::DiscretePoint(0, location.y - 1)
								));
				} else {
					ASSERT_THAT(neighbor_points, UnorderedElementsAre(
								fpmas::model::DiscretePoint(0, location.y - 1),
								fpmas::model::DiscretePoint(0, location.y + 1)
								));
				}
			}
		}

		void checkHorizontalLine(std::vector<fpmas::model::GridCell*> cells, int X) {

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
				if(location.x == 0) {
					ASSERT_THAT(neighbor_points, ElementsAre(
								fpmas::model::DiscretePoint(location.x + 1, 0)
								));
				} else if (location.x == X-1) {
					ASSERT_THAT(neighbor_points, ElementsAre(
								fpmas::model::DiscretePoint(location.x - 1, 0)
								));
				} else {
					ASSERT_THAT(neighbor_points, UnorderedElementsAre(
								fpmas::model::DiscretePoint(location.x - 1, 0),
								fpmas::model::DiscretePoint(location.x + 1, 0)
								));
				}
			}
		}

		/* Grid structure in a normal case */
		virtual void checkGrid(
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) = 0;

		void checkGridStructure(
				fpmas::api::model::SpatialModel<fpmas::model::GridCell>& grid_model,
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) {
			checkGridCells(grid_model, X, Y);
			if(X > 1 && Y > 1)
				checkGrid(cells, X, Y);
			else if(X == 1)
				checkVerticalLine(cells, Y);
			else if(Y == 1)
				checkHorizontalLine(cells, X);

		
			grid_model.graph().synchronize();
		}


};

class VonNeumannGridBuilderTest : public GridBuilderTestBase {
	protected:
		void checkGrid(
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) override {
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

TEST_F(VonNeumannGridBuilderTest, regular_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 4;
	int Y = X;
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, two_by_two_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 2;
	int Y = X;
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, four_by_four_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 4;
	int Y = X;
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, horizontal_line_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = grid_model.getMpiCommunicator().getSize() * 4;
	int Y = 1;
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, vertical_line_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<VonNeumannRange<VonNeumannGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 1;
	int Y = grid_model.getMpiCommunicator().getSize() * 4;
	VonNeumannGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
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

class MooreGridBuilderTest : public GridBuilderTestBase {
	private:
		void checkGrid(
				std::vector<fpmas::model::GridCell*> cells, int X, int Y) override {
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

TEST_F(MooreGridBuilderTest, regular_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<MooreRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = fpmas::communication::WORLD.getSize() * 2;
	int Y = X;
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, two_by_two_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<MooreRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 2;
	int Y = X;
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, four_by_four_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<MooreRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 4;
	int Y = X;
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, horizontal_line_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<MooreRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = grid_model.getMpiCommunicator().getSize() * 4;
	int Y = 1;
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
}

TEST_F(MooreGridBuilderTest, vertical_line_grid) {
	GridModel<fpmas::synchro::GhostMode, fpmas::model::GridCell,
		StaticEndCondition<MooreRange<MooreGrid<>>, 0, fpmas::model::GridCell>
			> grid_model;

	int X = 1;
	int Y = grid_model.getMpiCommunicator().getSize() * 4;
	MooreGridBuilder<> grid_builder(X, Y);

	ASSERT_EQ(grid_builder.width(), X);
	ASSERT_EQ(grid_builder.height(), Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(grid_model, cells, X, Y);
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

	int Y = fpmas::communication::WORLD.getSize() * 2;
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

class GridBuilderTest : public Test {
	protected:
		fpmas::model::GridModel<fpmas::synchro::GhostMode> model;
		fpmas::api::model::DiscreteCoordinate grid_width =
			10*model.getMpiCommunicator().getSize();
		fpmas::api::model::DiscreteCoordinate grid_height = 20;

		fpmas::model::VonNeumannGridBuilder<fpmas::model::GridCell> grid_builder {
				grid_width, grid_height
		};

		void SetUp() override {
			grid_builder.build(model);

		}
};

TEST_F(GridBuilderTest, random_initialization) {
	std::vector<fpmas::random::FPMAS_AGENT_RNG> random_generators;
	for(auto cell : model.cells())
		random_generators.push_back(cell->rd());

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

TEST_F(GridBuilderTest, init_sample) {
	using namespace fpmas::communication;

	std::size_t num_call = 0;
	std::set<fpmas::model::DistributedId> cell_ids;

	std::size_t sample_size = (grid_width*grid_height/2);
	grid_builder.initSample(
			sample_size,
			[&num_call, &cell_ids] (fpmas::model::GridCell* cell) {
				num_call++;
				cell_ids.insert(cell->node()->getId());
			});

	TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	num_call = all_reduce(int_mpi, num_call);

	ASSERT_EQ(num_call, sample_size);

	TypedMpi<decltype(cell_ids)> id_mpi(model.getMpiCommunicator());

	cell_ids = all_reduce(id_mpi, cell_ids, fpmas::utils::Concat());
	ASSERT_THAT(cell_ids, SizeIs(sample_size));
}

TEST_F(GridBuilderTest, init_sequence) {
	using namespace fpmas::communication;

	std::size_t num_call = 0;
	std::set<fpmas::model::DistributedId> cell_ids;
	std::set<unsigned long> assigned_items;

	std::size_t num_cells = grid_width * grid_height;
	std::vector<unsigned long> items(num_cells);
	for(std::size_t i = 0; i < num_cells; i++)
		items[i] = i;

	grid_builder.initSequence(
			items,
			[&num_call, &cell_ids, &assigned_items] (
				fpmas::model::GridCell* cell,
				const unsigned long& item) {
				num_call++;
				cell_ids.insert(cell->node()->getId());
				assigned_items.insert(item);
			});

	TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	num_call = all_reduce(int_mpi, num_call);

	ASSERT_EQ(num_call, num_cells);

	TypedMpi<decltype(cell_ids)> id_mpi(model.getMpiCommunicator());

	cell_ids = all_reduce(id_mpi, cell_ids, fpmas::utils::Concat());
	ASSERT_THAT(cell_ids, SizeIs(num_cells));

	TypedMpi<std::set<unsigned long>> item_mpi(model.getMpiCommunicator());
	assigned_items = all_reduce(item_mpi, assigned_items, fpmas::utils::Concat());
	ASSERT_THAT(assigned_items, SizeIs(num_cells));
}

class GridAgentBuilderTest : public GridBuilderTest {
	protected:
		static const std::size_t NUM_AGENT_BY_PROC = 20;
		fpmas::model::GridAgentBuilder<fpmas::model::GridCell> agent_builder;

		void SetUp() override {
			GridBuilderTest::SetUp();
			auto& agent_group = model.buildGroup(1);

			fpmas::model::UniformGridAgentMapping agent_mapping(
					grid_width, grid_height, NUM_AGENT_BY_PROC*model.getMpiCommunicator().getSize());
			fpmas::model::DefaultSpatialAgentFactory<TestGridAgent> agent_factory;

			agent_builder.build(
					model, {agent_group}, agent_factory, agent_mapping);
		}
};

TEST_F(GridAgentBuilderTest, random_initialization) {
	std::vector<fpmas::random::FPMAS_AGENT_RNG> random_generators;
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
	using namespace fpmas::communication;

	std::size_t num_call = 0;
	std::set<fpmas::model::DistributedId> agent_ids;

	std::size_t sample_size = (NUM_AGENT_BY_PROC/2)*model.getMpiCommunicator().getSize();
	agent_builder.initSample(
			sample_size,
			[&num_call, &agent_ids] (fpmas::api::model::GridAgent<fpmas::model::GridCell>* agent) {
				num_call++;
				agent_ids.insert(agent->node()->getId());
			});

	TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	num_call = all_reduce(int_mpi, num_call);

	ASSERT_EQ(num_call, sample_size);

	TypedMpi<decltype(agent_ids)> id_mpi(model.getMpiCommunicator());

	agent_ids = all_reduce(id_mpi, agent_ids, fpmas::utils::Concat());
	ASSERT_THAT(agent_ids, SizeIs(sample_size));
}

TEST_F(GridAgentBuilderTest, init_sequence) {
	using namespace fpmas::communication;

	std::size_t num_call = 0;
	std::set<fpmas::model::DistributedId> agent_ids;
	std::set<unsigned long> assigned_items;

	std::size_t num_agents = NUM_AGENT_BY_PROC * model.getMpiCommunicator().getSize();
	std::vector<unsigned long> items(num_agents);
	for(std::size_t i = 0; i < num_agents; i++)
		items[i] = i;

	agent_builder.initSequence(
			items,
			[&num_call, &agent_ids, &assigned_items] (
				fpmas::api::model::GridAgent<fpmas::model::GridCell>* agent,
				const unsigned long& item) {
				num_call++;
				agent_ids.insert(agent->node()->getId());
				assigned_items.insert(item);
			});

	TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	num_call = all_reduce(int_mpi, num_call);

	ASSERT_EQ(num_call, num_agents);

	TypedMpi<decltype(agent_ids)> id_mpi(model.getMpiCommunicator());

	agent_ids = all_reduce(id_mpi, agent_ids, fpmas::utils::Concat());
	ASSERT_THAT(agent_ids, SizeIs(num_agents));

	TypedMpi<std::set<unsigned long>> item_mpi(model.getMpiCommunicator());
	assigned_items = all_reduce(item_mpi, assigned_items, fpmas::utils::Concat());
	ASSERT_THAT(assigned_items, SizeIs(num_agents));
}
