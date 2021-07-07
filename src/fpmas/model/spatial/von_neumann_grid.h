#ifndef FPMAS_VON_NEUMANN_GRID_H
#define FPMAS_VON_NEUMANN_GRID_H

/** \file src/fpmas/model/spatial/von_neumann.h
 * VonNeumann neighborhood related objects.
 */

#include "grid_builder.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * Function object used to compute the [Manhattan
	 * distance](https://en.wikipedia.org/wiki/Taxicab_geometry) between two
	 * DiscretePoints.
	 */
	class ManhattanDistance {
		public:
			/**
			 * Computes the Manhattan distance between `p1` and `p2`.
			 *
			 * @return Manhattan distance between `p1` and `p2`
			 */
			DiscreteCoordinate operator()(
					const DiscretePoint& p1,
					const DiscretePoint& p2) const {
				return std::abs(p2.x - p1.x) + std::abs(p2.y - p1.y);
			}
	};

	/**
	 * CellNetworkBuilder implementation used to build a VonNeumann grid.
	 *
	 * The built grid is such the successors of each \GridCell correspond to
	 * \GridCells of its [VonNeumann
	 * neighborhood](https://en.wikipedia.org/wiki/Von_Neumann_neighborhood).
	 *
	 * Notice that this only has an impact on internal performances, notably
	 * concerning the DistributedMoveAlgorithm: \GridAgents' mobility and
	 * perception ranges are **not** limited to the "Von Neumann shape" of the
	 * Grid.
	 *
	 * @see VonNeumannRange
	 * @see MooreRange
	 */
	template<typename CellType = api::model::GridCell>
	class VonNeumannGridBuilder : public detail::GridBuilder<CellType> {
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
	void VonNeumannGridBuilder<CellType>::buildLocalGrid(
			api::model::SpatialModel<CellType>& model,
			detail::GridDimensions local_dimensions,
			typename detail::GridBuilder<CellType>::CellMatrix& cells
			) const {
		DiscreteCoordinate local_width = local_dimensions.width();
		DiscreteCoordinate local_height = local_dimensions.height();

		for(DiscreteCoordinate j = 0; j < local_width; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate i = local_height-1; i > 0; i--) {
				model.link(cells[i][j], cells[i-1][j], api::model::CELL_SUCCESSOR);
			}
		}
		for(DiscreteCoordinate i = 0; i < local_height; i++) {
			for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
				model.link(cells[i][j], cells[i][j+1], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate j = local_width-1; j > 0; j--) {
				model.link(cells[i][j], cells[i][j-1], api::model::CELL_SUCCESSOR);
			}
		}
	}
	template<typename CellType>
			void VonNeumannGridBuilder<CellType>::linkVerticalFrontiers(
					api::model::SpatialModel<CellType>& model,
					detail::GridDimensions local_dimensions,
					typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
					const std::array<std::vector<CellType*>, 2>& frontier
					) const {
				for(DiscreteCoordinate y = 0; y < this->height(); y++) {
					if(frontier[0].size() > 0) {
						// Left border
						model.link(
								local_cells[y][0], frontier[0][y],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
					if(frontier[1].size() > 0) {
						// Right border
						model.link(
								local_cells[y][local_dimensions.width()-1],
								frontier[1][y],
								SpatialModelLayers::CELL_SUCCESSOR
								);
					}
				}
			};

	template<typename CellType>
		void VonNeumannGridBuilder<CellType>::linkHorizontalFrontiers(
				api::model::SpatialModel<CellType>& model,
				detail::GridDimensions local_dimensions,
				typename detail::GridBuilder<CellType>::CellMatrix& local_cells,
				const std::array<std::vector<CellType*>, 2>& frontier
				) const {
			for(DiscreteCoordinate x = 0; x < this->width(); x++) {
				if(frontier[0].size() > 0) {
					// Bottom border
					model.link(
							local_cells[0][x], frontier[0][x],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
				if(frontier[1].size() > 0) {
					// Top border
					model.link(
							local_cells[local_dimensions.height()-1][x],
							frontier[1][x],
							SpatialModelLayers::CELL_SUCCESSOR
							);
				}
			}
		};



	/**
	 * VonNeumann GridConfig specialization, that might be used where a
	 * `GridConfig` template parameter is required.
	 */
	template<typename CellType = api::model::GridCell>
	using VonNeumannGrid = GridConfig<VonNeumannGridBuilder<CellType>, ManhattanDistance, CellType>;
}}
#endif
