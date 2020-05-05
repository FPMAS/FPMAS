#include "gtest/gtest.h"

#include "environment/grid.h"

using FPMAS::environment::grid::Grid;

TEST(Mpi_GridTest, build_test) {
	Grid grid(10, 5);

	ASSERT_EQ(grid.width, 10);
	ASSERT_EQ(grid.height, 5);
	// TODO : THIS NEEDS TO BE IMPROVED
	if(grid.getMpiCommunicator().getRank() == 0) {
		ASSERT_EQ(grid.getNodes().size(), 10 * 5);
	}
}
