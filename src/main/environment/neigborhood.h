#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "graph/base/graph.h"
#include "cell.h"

namespace FPMAS::environment::grid {

	static constexpr int neighborLayer(int d);

	template<
		typename grid_type,
		typename cell_type
	> class Neighborhood {
		protected:
			grid_type& localGrid;

		public:
			Neighborhood(grid_type& grid) : localGrid(grid) {}
			virtual void linkNeighbors(DistributedId nodeId, const cell_type*) = 0;
	};
	template<
		typename grid_type,
		typename cell_type,
		int Range
	> 
	class VonNeumann : public Neighborhood<grid_type, cell_type> {
			public:
				VonNeumann(grid_type& grid) : Neighborhood<grid_type, cell_type>(grid) {}
				void linkNeighbors(DistributedId nodeId, const cell_type* cell) override {
					for (int i = 1; i <= Range; i++) {
						if(cell->x() >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x() - i, cell->y()),
									neighborLayer(i)
									);
						}
						if(cell->x() < this->localGrid.width() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x() + i, cell->y()),
									neighborLayer(i)
									);
						}
						if(cell->y() >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x(), cell->y() - i),
									neighborLayer(i)
									);
						}
						if(cell->y() < this->localGrid.height() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell->x(), cell->y() + i),
									neighborLayer(i)
									);
						}				
					}
				}

	};
}
#endif
