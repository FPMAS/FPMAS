#ifndef NEIGHBORHOOD_H
#define NEIGHBORHOOD_H

#include "graph/base/graph.h"
#include "grid.h"
#include "cell.h"

namespace FPMAS::environment::grid {
	template<typename Grid> class Neighborhood {
		protected:
			Grid& localGrid;

		public:
			Neighborhood(Grid& grid) : localGrid(grid) {}
			virtual void linkNeighbors(int nodeId, const Cell&) = 0;
	};

	template<typename Grid, int Range>
		class VonNeumann : public Neighborhood<Grid> {
			private:
				ArcId arcId = 0;
		public:
			VonNeumann(Grid& grid) : Neighborhood<Grid>(grid) {}
			void linkNeighbors(int nodeId, const Cell& cell) override {
				for (int i = 1; i <= Range; i++) {
					if(cell.x() >= i) {
						this->localGrid.link(
								nodeId,
								this->localGrid.id(cell.x() - i, cell.y()),
								arcId++,
								Grid::Neighbor_Layer(i)
								);
					}
					if(cell.x() < this->localGrid.width() - i) {
						this->localGrid.link(
								nodeId,
								this->localGrid.id(cell.x() + i, cell.y()),
								arcId++,
								Grid::Neighbor_Layer(i)
								);
					}
					if(cell.y() >= i) {
						this->localGrid.link(
								nodeId,
								this->localGrid.id(cell.x(), cell.y() - i),
								arcId++,
								Grid::Neighbor_Layer(i)
								);
					}
					if(cell.y() < this->localGrid.height() - i) {
						this->localGrid.link(
								nodeId,
								this->localGrid.id(cell.x(), cell.y() + i),
								arcId++,
								Grid::Neighbor_Layer(i)
								);
					}				
				}
			}

	};
}
#endif
