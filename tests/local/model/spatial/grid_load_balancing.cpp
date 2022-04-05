#include "fpmas/model/spatial/grid_load_balancing.h"
#include "../../../mocks/communication/mock_communication.h"

using namespace testing;

namespace fpmas { namespace model {
	bool operator==(const GridDimensions& d1, const GridDimensions& d2) {
		return d1.getOrigin() == d2.getOrigin() && d1.getExtent() == d2.getExtent();
	}
}}

TEST(StripProcessMapping, width_sup_height) {
	MockMpiCommunicator<2, 4> comm;
	int width = 8;
	int height = 2;
	fpmas::model::StripProcessMapping mapping(width, height, comm);

	std::vector<fpmas::model::GridDimensions> grid_dimensions(4);
	for(int p = 0; p < comm.getSize(); p++) {
		grid_dimensions[p] = mapping.gridDimensions(p);
		for(int x = grid_dimensions[p].getOrigin().x; x < grid_dimensions[p].getExtent().x; x++)
			for(int y = grid_dimensions[p].getOrigin().y; y < grid_dimensions[p].getExtent().y; y++)
				ASSERT_EQ(mapping.process({x, y}), p);
	}
	ASSERT_THAT(grid_dimensions, UnorderedElementsAre(
				fpmas::model::GridDimensions({0, 0}, {2, 2}),
				fpmas::model::GridDimensions({2, 0}, {4, 2}),
				fpmas::model::GridDimensions({4, 0}, {6, 2}),
				fpmas::model::GridDimensions({6, 0}, {8, 2})
				));
	
}

TEST(StripProcessMapping, height_sup_width) {
	MockMpiCommunicator<2, 4> comm;
	int width = 2;
	int height = 8;
	fpmas::model::StripProcessMapping mapping(width, height, comm);

	std::vector<fpmas::model::GridDimensions> grid_dimensions(4);
	for(int p = 0; p < comm.getSize(); p++) {
		grid_dimensions[p] = mapping.gridDimensions(p);
		for(int x = grid_dimensions[p].getOrigin().x; x < grid_dimensions[p].getExtent().x; x++)
			for(int y = grid_dimensions[p].getOrigin().y; y < grid_dimensions[p].getExtent().y; y++)
				ASSERT_EQ(mapping.process({x, y}), p);
	}
	ASSERT_THAT(grid_dimensions, UnorderedElementsAre(
				fpmas::model::GridDimensions({0, 0}, {2, 2}),
				fpmas::model::GridDimensions({0, 2}, {2, 4}),
				fpmas::model::GridDimensions({0, 4}, {2, 6}),
				fpmas::model::GridDimensions({0, 6}, {2, 8})
				));
}

TEST(TreeProcessMapping, 4_processes) {
	MockMpiCommunicator<2, 4> comm;

	fpmas::model::TreeProcessMapping mapping(10, 10, comm);

	std::vector<fpmas::model::GridDimensions> grid_dimensions;
	for(int p = 0; p < comm.getSize(); p++) {
		grid_dimensions.push_back(mapping.gridDimensions(p));
		for(int x = grid_dimensions[p].getOrigin().x; x < grid_dimensions[p].getExtent().x; x++)
			for(int y = grid_dimensions[p].getOrigin().y; y < grid_dimensions[p].getExtent().y; y++)
				ASSERT_EQ(mapping.process({x, y}), p);
	}
}

TEST(TreeProcessMapping, 16_processes) {
	MockMpiCommunicator<2, 16> comm;

	fpmas::model::TreeProcessMapping mapping(8, 8, comm);
	mapping.process({0, 0});

	std::vector<int> processes;
	std::vector<int> expected;
	for(int i = 0; i < 8; i++) {
		for(int j = 0; j < 8; j++) {
			processes.push_back(mapping.process({i, j}));
		}
	}
	for(int i = 0; i < 16; i++)
		for(int k = 0; k < 4; k++)
			expected.push_back(i);
	ASSERT_THAT(processes, UnorderedElementsAreArray(expected));
}
