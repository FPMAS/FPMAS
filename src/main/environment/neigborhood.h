#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "graph/base/graph.h"
#include "cell.h"

namespace FPMAS::environment::grid {

	template<
		typename grid_type
	> class Neighborhood {
		public:
			const unsigned int range;
			Neighborhood(unsigned int range) : range(range) {}
			virtual void linkNeighbors(grid_type& grid, DistributedId nodeId, const CellBase*) = 0;
	};

	template<
		typename grid_type
	> 
	class VonNeumann : public Neighborhood<grid_type> {
			public:
				VonNeumann(unsigned int range)
					: Neighborhood<grid_type>(range) {}

				void linkNeighbors(grid_type& grid, DistributedId nodeId, const CellBase* cell) override {
					for (int i = 1; i <= this->range; i++) {
						if(cell->x >= i) {
							grid.link(
									nodeId,
									grid.id(cell->x - i, cell->y),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->x < grid.width - i) {
							grid.link(
									nodeId,
									grid.id(cell->x + i, cell->y),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->y >= i) {
							grid.link(
									nodeId,
									grid.id(cell->x, cell->y - i),
									NEIGHBOR_CELL(i)
									);
						}
						if(cell->y < grid.height - i) {
							grid.link(
									nodeId,
									grid.id(cell->x, cell->y + i),
									NEIGHBOR_CELL(i)
									);
						}				
					}
				}

	};
}
#endif
