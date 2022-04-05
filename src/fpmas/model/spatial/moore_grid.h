#ifndef FPMAS_MOORE_GRID_H
#define FPMAS_MOORE_GRID_H

/** \file src/fpmas/model/spatial/moore_grid.h
 * Moore grid related objects.
 */

#include "grid_builder.h"

namespace fpmas { namespace model {

	/**
	 * Function object used to compute the [Chebyshev
	 * distance](https://en.wikipedia.org/wiki/Chebyshev_distance) between two
	 * DiscretePoints.
	 */
	class ChebyshevDistance {
		public:
			/**
			 * Computes the Chebyshev distance between `p1` and `p2`.
			 *
			 * @return Chebyshev distance between `p1` and `p2`
			 */
			DiscreteCoordinate operator()(
					const DiscretePoint& p1,
					const DiscretePoint& p2) const {
				return std::max(std::abs(p2.x - p1.x), std::abs(p2.y - p1.y));
			}
	};

	/**
	 * CellNetworkBuilder implementation used to build a Moore grid.
	 *
	 * The built grid is such the successors of each \GridCell correspond to
	 * \GridCells of its [Moore
	 * neighborhood](https://en.wikipedia.org/wiki/Moore_neighborhood).
	 *
	 * Notice that this only has an impact on internal performances, notably
	 * concerning the DistributedMoveAlgorithm: \GridAgents' mobility and
	 * perception ranges are **not** limited to the "Moore shape" of the
	 * Grid.
	 *
	 * @see VonNeumannRange
	 * @see MooreRange
	 */
	template<typename CellType = model::GridCell>
	class MooreGridBuilder : public detail::GridBuilder<CellType> {
		private:

			/**
			 * Links cells on the SUCCESSOR layer so that each cell is linked
			 * to its Moore neighborhood.
			 *
			 * Missing \DISTANT cells are not considered yet, so neighborhoods
			 * of cells at borders are not completed yet.
			 *
			 * @param model model in which cells are built
			 * @param local_dimensions dimension of the local grid, that is
			 * only a subpart of the global grid
			 * @param cells matrix containing \LOCAL cells built on the current
			 * process
			 */
			void buildLocalGrid(
					api::model::SpatialModel<CellType>& model,
					GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& cells
					) const override;

			/**
			 * Links \DISTANT cells in `frontier` to `local_cells` in order to
			 * complete Moore neighborhoods of border cells.
			 *
			 * @param model model into which SUCCESSOR links must be built
			 * @param local_dimensions grid dimensions describing the grid
			 * part built on the current process
			 * @param local_cells matrix containing \LOCAL cells built on
			 * the current process
			 * @param frontier a list of \DISTANT cells that can be linked
			 * to `local_cells`
			 */
			void linkFrontiers(
					api::model::SpatialModel<CellType>& model,
					GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					std::vector<CellType*>& frontier
					) const override;
		public:
			using detail::GridBuilder<CellType>::GridBuilder;
	};

	template<typename CellType>
	void MooreGridBuilder<CellType>::buildLocalGrid(
			api::model::SpatialModel<CellType>& model,
			GridDimensions local_dimensions,
			typename detail::GridBuilder<CellType>::CellMatrix& cells
			) const {
		DiscreteCoordinate local_width = local_dimensions.width();
		DiscreteCoordinate local_height = local_dimensions.height();

		// VonNeumann links (horizontal direction)
		for(DiscreteCoordinate j = 0; j < local_width; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate i = local_height-1; i > 0; i--) {
				model.link(cells[i][j], cells[i-1][j], api::model::CELL_SUCCESSOR);
			}
		}
		
		// VonNeumann links (vertical direction)
		for(DiscreteCoordinate i = 0; i < local_height; i++) {
			for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
				model.link(cells[i][j], cells[i][j+1], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate j = local_width-1; j > 0; j--) {
				model.link(cells[i][j], cells[i][j-1], api::model::CELL_SUCCESSOR);
			}
		}

		// Links NE corners
		for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j+1], api::model::CELL_SUCCESSOR);
			}
		}

		// Links NW corners
		for(DiscreteCoordinate j = 1; j < local_width; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j-1], api::model::CELL_SUCCESSOR);
			}
		}

		// Links SE corners
		for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
			for(DiscreteCoordinate i = 1; i < local_height; i++) {
				model.link(cells[i][j], cells[i-1][j+1], api::model::CELL_SUCCESSOR);
			}
		}

		// Links SW corners
		for(DiscreteCoordinate j = 1; j < local_width; j++) {
			for(DiscreteCoordinate i = 1; i < local_height; i++) {
				model.link(cells[i][j], cells[i-1][j-1], api::model::CELL_SUCCESSOR);
			}
		}
	}
	
	template<typename CellType>
			void MooreGridBuilder<CellType>::linkFrontiers(
					api::model::SpatialModel<CellType>& model,
					GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					std::vector<CellType*>& frontier
					) const {
				for(auto cell : frontier) {
					if(cell->location().y == local_dimensions.getExtent().y) {
						// top

						// X coordinate of the cell below (index in the local_cells
						// matrix)
						DiscreteCoordinate mid_x = cell->location().x - local_dimensions.getOrigin().x;
						if(mid_x >= 0 && mid_x < local_dimensions.width())
							// For corners, the mid_x value is not in the local
							// matrix
							model.link(
									local_cells[local_dimensions.height()-1][mid_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// X coordinate the cell below left (index in the local_cells
						// matrix)
						DiscreteCoordinate left_x = mid_x-1;
						if(left_x >= 0)
							model.link(
									local_cells[local_dimensions.height()-1][left_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// X coordinate of the cell below right (index in the
						// local_cells matrix)
						DiscreteCoordinate right_x = mid_x+1;
						if(right_x < local_dimensions.width())
							model.link(
									local_cells[local_dimensions.height()-1][right_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);
					}
					else if(cell->location().y == local_dimensions.getOrigin().y-1) {
						// bottom

						// X coordinate of the cell above (index in the local_cells
						// matrix)
						DiscreteCoordinate mid_x = cell->location().x - local_dimensions.getOrigin().x;
						if(mid_x >= 0 && mid_x < local_dimensions.width())
							model.link(
									local_cells[0][mid_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// X coordinate the cell above left (index in the local_cells
						// matrix)
						DiscreteCoordinate left_x = mid_x-1;
						if(left_x >= 0)
							model.link(
									local_cells[0][left_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// X coordinate of the cell above right (index in the
						// local_cells matrix)
						DiscreteCoordinate right_x = mid_x+1;
						if(right_x < local_dimensions.width())
							model.link(
									local_cells[0][right_x],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);
					}
					else if(cell->location().x == local_dimensions.getOrigin().x-1) {
						// left

						// Y coordinate of the cell left (index in the local_cells
						// matrix)
						DiscreteCoordinate left_y = cell->location().y - local_dimensions.getOrigin().y;
						if(left_y >= 0 && left_y < local_dimensions.height())
							model.link(
									local_cells[left_y][0],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// Y coordinate the cell below left (index in the local_cells
						// matrix)
						DiscreteCoordinate bottom_y = left_y-1;
						if(bottom_y >= 0)
							model.link(
									local_cells[bottom_y][0],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// y coordinate of the cell above left (index in the
						// local_cells matrix)
						DiscreteCoordinate top_y = left_y+1;
						if(top_y < local_dimensions.height())
							model.link(
									local_cells[top_y][0],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);
					}
					else if(cell->location().x == local_dimensions.getExtent().x) {
						// right

						// Y coordinate of the cell left (index in the local_cells
						// matrix)
						DiscreteCoordinate right_y = cell->location().y - local_dimensions.getOrigin().y;
						if(right_y >= 0 && right_y < local_dimensions.height())
							model.link(
									local_cells[right_y][local_dimensions.width()-1],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// Y coordinate the cell below right (index in the local_cells
						// matrix)
						DiscreteCoordinate bottom_y = right_y-1;
						if(bottom_y >= 0)
							model.link(
									local_cells[bottom_y][local_dimensions.width()-1],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);

						// y coordinate of the cell above left (index in the
						// local_cells matrix)
						DiscreteCoordinate top_y = right_y+1;
						if(top_y < local_dimensions.height())
							model.link(
									local_cells[top_y][local_dimensions.width()-1],
									cell,
									SpatialModelLayers::CELL_SUCCESSOR
									);
					}
				}
			}
	/**
	 * Moore GridConfig specialization, that might be used where a
	 * `GridConfig` template parameter is required.
	 */
	template<typename CellType = model::GridCell>
	using MooreGrid = GridConfig<MooreGridBuilder<CellType>, ChebyshevDistance, CellType>;

}}
#endif
