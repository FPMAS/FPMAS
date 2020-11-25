#include "fpmas/model/grid.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "gmock/gmock.h"
#include "test_agents.h"
#include "../mocks/random/mock_random.h"
#include "../mocks/model/mock_grid.h"

using namespace testing;
using fpmas::model::VonNeumannGridBuilder;

class VonNeumannGridBuilderTest : public ::testing::Test {
	protected:
		fpmas::model::SpatialModel<fpmas::synchro::GhostMode> grid_model {0, 0};

		void checkGridStructure(std::vector<fpmas::api::model::GridCell*> cells, int X, int Y) {
			fpmas::communication::TypedMpi<std::vector<fpmas::model::DiscretePoint>> mpi(
					fpmas::communication::MpiCommunicator::WORLD);

			std::vector<fpmas::model::DiscretePoint> local_cell_coordinates;
			for(auto cell : grid_model.cells())
				local_cell_coordinates.push_back(
						dynamic_cast<fpmas::api::model::GridCell*>(cell)->location());

			std::vector<std::vector<fpmas::api::model::DiscretePoint>> recv_coordinates
				= mpi.gather(local_cell_coordinates, 0);

			FPMAS_ON_PROC(fpmas::communication::MpiCommunicator::WORLD, 0) {
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

			for(auto grid_cell : cells) {
				auto location = grid_cell->location();
				auto cell_successors = grid_cell->neighborhood();
				std::vector<fpmas::model::DiscretePoint> neighbor_points;
				for(auto cell : cell_successors)
					neighbor_points.push_back(
							dynamic_cast<fpmas::api::model::GridCell*>(cell)->location());

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
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, 1),
									fpmas::model::DiscretePoint(1, 0)));
					} else if(location.y == Y-1) {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, Y-2),
									fpmas::model::DiscretePoint(1, Y-1)));
					} else {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(0, location.y-1),
									fpmas::model::DiscretePoint(0, location.y+1),
									fpmas::model::DiscretePoint(1, location.y)));
					}
				} else if (location.x == X-1) {
					if(location.y == 0) {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-2, 0),
									fpmas::model::DiscretePoint(X-1, 1)));
					} else if(location.y == Y-1) {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, Y-2),
									fpmas::model::DiscretePoint(X-2, Y-1)));
					} else {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(X-1, location.y-1),
									fpmas::model::DiscretePoint(X-1, location.y+1),
									fpmas::model::DiscretePoint(X-2, location.y)));
					}
				} else {
					if(location.y == 0) {
						ASSERT_THAT(neighbor_points, UnorderedElementsAre(
									fpmas::model::DiscretePoint(location.x-1, 0),
									fpmas::model::DiscretePoint(location.x+1, 0),
									fpmas::model::DiscretePoint(location.x, 1)));
					} else if (location.y == Y-1) {
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
	VonNeumannGridBuilder null(0, 0);

	null.build(grid_model);
	ASSERT_THAT(grid_model.cells(), IsEmpty());
}

TEST_F(VonNeumannGridBuilderTest, build_height_sup_width) {
	int X = fpmas::communication::MpiCommunicator::WORLD.getSize() * 10;
	int Y = 2*X;
	VonNeumannGridBuilder grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(cells, X, Y);
}

TEST_F(VonNeumannGridBuilderTest, build_width_sup_height) {
	int Y = fpmas::communication::MpiCommunicator::WORLD.getSize() * 10;
	int X = 2*Y;
	VonNeumannGridBuilder grid_builder(X, Y);

	auto cells = grid_builder.build(grid_model);

	checkGridStructure(cells, X, Y);
}

class UniformAgentMappingTest : public Test {
	protected:
		fpmas::model::UniformAgentMapping mapping {10, 10, 30};
		std::array<NiceMock<MockGridCell>, 100> cells;

		void SetUp() override {
			for(fpmas::model::DiscreteCoordinate x = 0; x < 10; x++)
				for(fpmas::model::DiscreteCoordinate y = 0; y < 10; y++)
					ON_CALL(cells[10*x+y], location)
						.WillByDefault(Return(fpmas::model::DiscretePoint {x, y}));
		}
};

TEST_F(UniformAgentMappingTest, consistency_across_processes) {
	std::array<std::size_t, 100> counts;
	for(std::size_t i = 0; i < 100; i++)
		counts[i] = mapping.countAt(&cells[i]);
	std::size_t total_count = std::accumulate(counts.begin(), counts.end(), 0);
	// First, the mapping must generate exactly 30 agent on the 10x10 grid
	ASSERT_EQ(total_count, 30);

	fpmas::communication::TypedMpi<std::array<std::size_t, 100>> mpi
		(fpmas::communication::MpiCommunicator::WORLD);
	std::array<std::size_t, 100> proc_0_counts = mpi.bcast(counts, 0);

	// The mapping is generated intependently on each process.
	// However, it spite of randomness, **all processes** are required to
	// generate **exactly** the same mapping.
	ASSERT_THAT(counts, ElementsAreArray(proc_0_counts));
}
