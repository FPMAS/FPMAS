#include "gtest/gtest.h"

#include "utils/test.h"
#include "environment/grid.h"

using FPMAS::test::ASSERT_CONTAINS;
using FPMAS::environment::grid::Grid;
using FPMAS::environment::grid::Cell;
using FPMAS::environment::grid::neighborLayer;

using FPMAS::environment::grid::VonNeumann;

template<int Range> void TestVonNeumannLevel(Grid<VonNeumann, Range>& grid, int level) {
	for(auto node : grid.getNodes()) {
		const auto& cell
			= dynamic_cast<typename Grid<VonNeumann, Range>::cell_type&>(
					*node.second->data()->read()
			);
		const auto* node_ptr = node.second;
		int x = cell.x();
		int y = cell.y();
		const auto& in = node_ptr->layer(neighborLayer(Grid<VonNeumann, Range>::userLayers, level)).inNeighbors();
		const auto& out = node_ptr->layer(neighborLayer(Grid<VonNeumann, Range>::userLayers, level)).outNeighbors();

		/*
		 * Test corners
		 */
		if((x < level && y < level)
			|| (x >= grid.width() - level && y < level)
			|| (x < level && y >= grid.height() - level)
			|| (x >= grid.width() - level && y >= grid.height() - level)
			) {
			ASSERT_EQ(in.size(), 2);
			ASSERT_EQ(out.size(), 2);
			if(x < level) {
				ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), in);
				ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), out);
			}
			if(x >= grid.width() - level) {
				ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), in);
				ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), out);
			}
			if(y<level) {
				ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), in);
				ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), out);
			}
			if(y >= grid.height() - level) {
				ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), in);
				ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), out);
			}
		}
		// Left side
		else if(x<level) {
			ASSERT_EQ(in.size(), 3);
			ASSERT_EQ(out.size(), 3);

			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), out);

		}
		// Right side
		else if(x>=grid.width()-level) {
			ASSERT_EQ(in.size(), 3);
			ASSERT_EQ(out.size(), 3);

			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), out);
		}
		// Bottom side
		else if(y<level) {
			ASSERT_EQ(in.size(), 3);
			ASSERT_EQ(out.size(), 3);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), out);
		}
		// Top side
		else if(y>=grid.height()-level) {
			ASSERT_EQ(in.size(), 3);
			ASSERT_EQ(out.size(), 3);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), out);
		}
		else {
			ASSERT_EQ(in.size(), 4);
			ASSERT_EQ(out.size(), 4);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y+level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x, y-level)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x+level, y)), out);

			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), in);
			ASSERT_CONTAINS(grid.getNode(grid.id(x-level, y)), out);
		}
	}

}

template<int Range> void TestVonNeumann(Grid<VonNeumann, Range>& grid) {
	for(int i = 1; i <= Range; i++) {
		TestVonNeumannLevel<Range>(grid, i);
	}
}

TEST(Mpi_GridTest, default_von_neumann) {
	Grid grid(10, 10);
	TestVonNeumann<1>(grid);
}


TEST(Mpi_GridTest, von_neumann_2) {
	Grid<VonNeumann, 2> grid(10, 10);
	TestVonNeumann<2>(grid);
}
