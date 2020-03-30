#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "graph/base/graph.h"
#include "cell.h"

namespace FPMAS::environment::grid {

	template<
		typename grid_type
	> class Neighborhood {
		protected:
			const unsigned int range;
			grid_type& localGrid;

		public:
			Neighborhood(grid_type& grid, unsigned int range) : localGrid(grid), range(range) {}
			virtual void linkNeighbors(DistributedId nodeId, const CellBase*) = 0;
	};

	template<
		typename grid_type
	> 
	class VonNeumann : public Neighborhood<grid_type> {
			public:
				VonNeumann(grid_type& grid, unsigned int range)
					: Neighborhood<grid_type>(grid, range) {}
				void linkNeighbors(DistributedId nodeId, const CellBase* cell) override {
					for (int i = 1; i <= this->range; i++) {
						if(cell->x >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x - i, cell->y),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->x < this->localGrid.width() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x + i, cell->y),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->y >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x, cell->y - i),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->y < this->localGrid.height() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x, cell->y + i),
									NEIGHBOR_CELL(i)
									);
						}				
					}
				}

	};
}
#endif
