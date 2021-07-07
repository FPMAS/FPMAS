#ifndef FPMAS_MOORE_GRID_H
#define FPMAS_MOORE_GRID_H

/** \file src/fpmas/model/spatial/moore.h
 * Moore neighborhood related objects.
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
	template<typename CellType = api::model::GridCell>
	class MooreGridBuilder : public detail::GridBuilder<CellType> {
		private:

			void buildLocalGrid(
					api::model::SpatialModel<CellType>& model,
					detail::GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& cells
					) const override;

			void linkVerticalFrontiers(
					api::model::SpatialModel<CellType>& model,
					detail::GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					const std::array<std::vector<CellType*>, 2>& frontier
					) const override;

			void linkHorizontalFrontiers(
					api::model::SpatialModel<CellType>& model,
					detail::GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					const std::array<std::vector<CellType*>, 2>& frontier
					) const override;

		public:
			using detail::GridBuilder<CellType>::GridBuilder;
	};

	template<typename CellType>
	void MooreGridBuilder<CellType>::buildLocalGrid(
			api::model::SpatialModel<CellType>& model,
			detail::GridDimensions local_dimensions,
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
			void MooreGridBuilder<CellType>::linkVerticalFrontiers(
					api::model::SpatialModel<CellType>& model,
					detail::GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					const std::array<std::vector<CellType*>, 2>& frontier
					) const {
				// VonNeumann links
				for(DiscreteCoordinate y = 0; y < this->height(); y++) {
					if(frontier[0].size() > 0) {
						model.link(
								local_cells[y][0], frontier[0][y],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
					if(frontier[1].size() > 0) {
						model.link(
								local_cells[y][local_dimensions.width()-1],
								frontier[1][y],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
				}
				// North corners
				for(DiscreteCoordinate y = 0; y < this->height()-1; y++) {
					if(frontier[0].size() > 0) {
						// Left frontier
						model.link(
								local_cells[y][0], frontier[0][y+1],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
					if(frontier[1].size() > 0) {
						// Right frontier
						model.link(
								local_cells[y][local_dimensions.width()-1],
								frontier[1][y+1],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
				}
				// South corners
				for(DiscreteCoordinate y = 1; y < this->height(); y++) {
					if(frontier[0].size() > 0) {
						// Left frontier
						model.link(
								local_cells[y][0], frontier[0][y-1],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
					if(frontier[1].size() > 0) {
						// Right frontier
						model.link(
								local_cells[y][local_dimensions.width()-1],
								frontier[1][y-1],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
				}
			};

	template<typename CellType>
		void MooreGridBuilder<CellType>::linkHorizontalFrontiers(
				api::model::SpatialModel<CellType>& model,
				detail::GridDimensions local_dimensions,
				typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
				const std::array<std::vector<CellType*>, 2>& frontier
				) const {
			// Von Neumann links
			for(DiscreteCoordinate x = 0; x < this->width(); x++) {
				if(frontier[0].size() > 0) {
					model.link(
							local_cells[0][x], frontier[0][x],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
				if(frontier[1].size() > 0) {
					model.link(
							local_cells[local_dimensions.height()-1][x],
							frontier[1][x],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
			}
			// East corners
			for(DiscreteCoordinate x = 0; x < this->width()-1; x++) {
				if(frontier[0].size() > 0) {
					// Bottom frontier
					model.link(
							local_cells[0][x], frontier[0][x+1],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
				if(frontier[1].size() > 0) {
					// Top frontier
					model.link(
							local_cells[local_dimensions.height()-1][x],
							frontier[1][x+1],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
			}
			// West corners
			for(DiscreteCoordinate x = 1; x < this->width(); x++) {
				if(frontier[0].size() > 0) {
					// Left frontier
					model.link(
							local_cells[0][x], frontier[0][x-1],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
				if(frontier[1].size() > 0) {
					// Right frontier
					model.link(
							local_cells[local_dimensions.height()-1][x],
							frontier[1][x-1],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
			}
		};

	/**
	 * Moore GridConfig specialization, that might be used where a
	 * `GridConfig` template parameter is required.
	 */
	template<typename CellType = api::model::GridCell>
	using MooreGrid = GridConfig<MooreGridBuilder<CellType>, ChebyshevDistance, CellType>;

}}
#endif
