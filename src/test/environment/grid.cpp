#include "gtest/gtest.h"
#include "environment/grid.h"

using FPMAS::environment::grid::Grid;
using FPMAS::environment::grid::GridLayer;
using FPMAS::environment::grid::Cell;

TEST(Mpi_GridTest, build_test) {
	Grid<10, 10> grid;

	for(auto node : grid.getNodes()) {
		const Cell& cell = dynamic_cast<Cell&>(*node.second->data()->read());
		const auto* node_ptr = node.second;
		if(cell.x() == 0 && cell.y() == 0) {
			ASSERT_EQ(node_ptr->layer(GridLayer).getIncomingArcs().size(), 2);
			ASSERT_EQ(node_ptr->layer(GridLayer).getOutgoingArcs().size(), 2);
		}
		if(cell.x() > 0 && cell.x() < 9 && cell.y() > 0 && cell.y() < 9) {
			ASSERT_EQ(node_ptr->layer(GridLayer).getIncomingArcs().size(), 4);
			ASSERT_EQ(node_ptr->layer(GridLayer).getOutgoingArcs().size(), 4);

		}
	}
	

}
