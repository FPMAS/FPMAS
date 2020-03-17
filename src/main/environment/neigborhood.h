#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "graph/base/graph.h"
#include "grid.h"
#include "cell.h"

namespace FPMAS::environment::grid {
	template<
		typename grid_type,
		typename cell_type
	> class Neighborhood {
		protected:
			grid_type& localGrid;

		public:
			Neighborhood(grid_type& grid) : localGrid(grid) {}
			virtual void linkNeighbors(int nodeId, const cell_type&) = 0;
	};
	template<
		typename grid_type,
		typename cell_type,
		int Range
	> 
	class VonNeumann : public Neighborhood<grid_type, cell_type> {
			private:
				ArcId arcId = 0;
			public:
				VonNeumann(grid_type& grid) : Neighborhood<grid_type, cell_type>(grid) {}
				void linkNeighbors(int nodeId, const cell_type& cell) override {
					for (int i = 1; i <= Range; i++) {
						if(cell.x() >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell.x() - i, cell.y()),
									arcId++,
									grid_type::Neighbor_Layer(i)
									);
						}
						if(cell.x() < this->localGrid.width() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell.x() + i, cell.y()),
									arcId++,
									grid_type::Neighbor_Layer(i)
									);
						}
						if(cell.y() >= i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell.x(), cell.y() - i),
									arcId++,
									grid_type::Neighbor_Layer(i)
									);
						}
						if(cell.y() < this->localGrid.height() - i) {
							this->localGrid.link(
									nodeId,
									this->localGrid.id(cell.x(), cell.y() + i),
									arcId++,
									grid_type::Neighbor_Layer(i)
									);
						}				
					}
				}

	};
}
#endif
