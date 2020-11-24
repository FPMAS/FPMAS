#include "fpmas/model/grid.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "gmock/gmock.h"
#include "test_agents.h"

using namespace testing;
using fpmas::model::VonNeumannNeighborhood;

class VonNeumannNeighborhoodTest : public ::testing::Test {
	protected:
		fpmas::model::SpatialModel<fpmas::synchro::GhostMode> grid_model {0, 0};
		fpmas::model::GridCellFactory<> cell_factory;

};

TEST_F(VonNeumannNeighborhoodTest, trivial) {
	VonNeumannNeighborhood null(cell_factory, 0, 0);

	null.build(grid_model);
	ASSERT_THAT(grid_model.cells(), IsEmpty());
}

TEST_F(VonNeumannNeighborhoodTest, build) {
	int X = fpmas::communication::MpiCommunicator::WORLD.getSize() * 10;
	int Y = 2*X;
	VonNeumannNeighborhood grid_builder(cell_factory, X, Y);

	fpmas::communication::TypedMpi<std::vector<fpmas::model::DiscretePoint>> mpi(
			fpmas::communication::MpiCommunicator::WORLD);

	grid_builder.build(grid_model);

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

	for(auto cell : grid_model.cells()) {
		fpmas::api::model::GridCell* grid_cell
			= dynamic_cast<fpmas::api::model::GridCell*>(cell);
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
