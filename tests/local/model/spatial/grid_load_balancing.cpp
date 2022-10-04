#include "fpmas/model/spatial/grid_load_balancing.h"
#include "../../../mocks/communication/mock_communication.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>

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

class FastProcessMapping : public Test {
	protected:
		void check_grid(
				const fpmas::model::FastProcessMapping& mapping,
				int comm_size, int width, int height) {
			std::vector<std::vector<int>> check_mapping;
			check_mapping.resize(width);
			for(int x = 0; x < width; x++) {
				check_mapping[x].resize(height);
				for(int y = 0; y < height; y++)
					check_mapping[x][y] = -1;
			}
			for(int p = 0; p < comm_size; p++) {
				auto grid_dimensions = mapping.gridDimensions(p);
				for(int x = grid_dimensions.getOrigin().x; x < grid_dimensions.getExtent().x; x++) {
					for(int y = grid_dimensions.getOrigin().y; y < grid_dimensions.getExtent().y; y++) {
						ASSERT_EQ(mapping.process({x, y}), p);
						check_mapping[x][y] = p;
					}
				}
			}
			for(auto item : check_mapping)
				for (auto process : item)
					// Check that each coordinate is properly associated to a process
					ASSERT_GE(process, 0);


		}
};
TEST_F(FastProcessMapping, 4_processes_16x16) {
	MockMpiCommunicator<2, 4> comm;

	int width = 16;
	int height = 16;
	fpmas::model::FastProcessMapping mapping(width, height, comm);
	check_grid(mapping, comm.getSize(), width, height);

	ASSERT_EQ(mapping.n(), 2);
	ASSERT_EQ(mapping.p(), 2);
	for(int i = 0; i < 4; i++) {
		ASSERT_EQ(mapping.gridDimensions(i).height(), 8);
		ASSERT_EQ(mapping.gridDimensions(i).width(), 8);
	}

}

TEST_F(FastProcessMapping, 4_processes_64x16) {
	MockMpiCommunicator<2, 4> comm;

	int width = 64;
	int height = 16;
	fpmas::model::FastProcessMapping mapping(width, height, comm);
	check_grid(mapping, comm.getSize(), width, height);

	ASSERT_EQ(mapping.n(), 4);
	ASSERT_EQ(mapping.p(), 1);
	for(int i = 0; i < 4; i++) {
		ASSERT_EQ(mapping.gridDimensions(i).height(), 16);
		ASSERT_EQ(mapping.gridDimensions(i).width(), 16);
	}
}

TEST_F(FastProcessMapping, 4_processes_16x64) {
	MockMpiCommunicator<2, 4> comm;

	int width = 16;
	int height = 64;
	fpmas::model::FastProcessMapping mapping(width, height, comm);
	check_grid(mapping, comm.getSize(), width, height);

	ASSERT_EQ(mapping.n(), 1);
	ASSERT_EQ(mapping.p(), 4);
	for(int i = 0; i < 4; i++) {
		ASSERT_EQ(mapping.gridDimensions(i).height(), 16);
		ASSERT_EQ(mapping.gridDimensions(i).width(), 16);
	}

}

TEST_F(FastProcessMapping, 8_processes_101x101) {
	MockMpiCommunicator<2, 8> comm;

	int width = 101;
	int height = 101;
	fpmas::model::FastProcessMapping mapping(width, height, comm);
	check_grid(mapping, comm.getSize(), width, height);

	std::array<int, 2> n_p = {mapping.n(), mapping.p()};
	ASSERT_THAT(n_p, UnorderedElementsAre(4, 2));
	for(int i = 0; i < 4; i++) {
		ASSERT_GE(
				mapping.gridDimensions(i).height()*mapping.gridDimensions(i).width(),
				1250
				);
	}
}
