#ifndef FPMAS_VON_NEUMANN_H
#define FPMAS_VON_NEUMANN_H

/** \file src/fpmas/model/spatial/von_neumann.h
 * VonNeumann neighborhood related objects.
 */

#include "grid.h"

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
	class VonNeumannGridBuilder : public api::model::CellNetworkBuilder<api::model::GridCell> {
		private:
			typedef std::vector<std::vector<api::model::GridCell*>> CellMatrix;
			void allocate(CellMatrix& matrix, DiscreteCoordinate width, DiscreteCoordinate height) const;

			api::model::GridCellFactory& cell_factory;
			DiscreteCoordinate width;
			DiscreteCoordinate height;

			CellMatrix buildLocalGrid(
					api::model::GridModel& model,
					DiscreteCoordinate min_x, DiscreteCoordinate max_x,
					DiscreteCoordinate min_y, DiscreteCoordinate max_y) const;

		public:
			/**
			 * GridCellFactory used by default, building GridCell instances.
			 */
			static GridCellFactory<GridCell> default_cell_factory;

			/**
			 * VonNeumannGridBuilder constructor.
			 *
			 * The `cell_factory` parameter allows to build the grid using
			 * custom GridCell extensions (with extra user defined behaviors for
			 * example).
			 *
			 * @param cell_factory custom cell factory, that will be used
			 * instead of default_cell_factory
			 * @param width width of the grid
			 * @param height height of the grid
			 */
			VonNeumannGridBuilder(
					api::model::GridCellFactory& cell_factory,
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: cell_factory(cell_factory), width(width), height(height) {}
			/**
			 * VonNeumannGridBuilder constructor.
			 *
			 * The grid will be built using the default_cell_factory.
			 *
			 * @param width width of the grid
			 * @param height height of the grid
			 */
			VonNeumannGridBuilder(
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: VonNeumannGridBuilder(default_cell_factory, width, height) {}

			/**
			 * Builds a VonNeumann grid into the specified `spatial_model`.
			 *
			 * The process is distributed so that each available process
			 * instantiates a part of the global grid.
			 *
			 * The size of the global grid is equal to `width x height`,
			 * according to the parameters specified in the constructor. The
			 * origin of the grid is considered at (0, 0), and so the opposite
			 * corner is at (width-1, height-1).
			 *
			 * Each GridCell is built using the `cell_factory` specified in the
			 * constructor, the `default_cell_factory` otherwise.
			 *
			 * The successors of each GridCell correspond to \GridCells
			 * contained in its VonNeumann neighborhood (excluding the GridCell
			 * itself).
			 *
			 * @param spatial_model spatial model in which cells will be added
			 * @return cells built on the current process
			 */
			std::vector<api::model::GridCell*> build(
					api::model::SpatialModel<api::model::GridCell>& spatial_model) const override;
	};

	/**
	 * VonNeumann GridConfig specialization, that might be used where a
	 * `GridConfig` template parameter is required.
	 */
	typedef GridConfig<VonNeumannGridBuilder, ManhattanDistance> VonNeumannGrid;

	/**
	 * VonNeumannRange perimeter function object.
	 *
	 * **Must** be explicitly specialized for each available GridConfig.
	 *
	 * See GridRangeConfig::Perimeter for more information about what a
	 * "perimeter" is.
	 */
	template<typename GridConfig>
		struct VonNeumannRangePerimeter {};

	/**
	 * VonNeumann GridRangeConfig specialization, that might be used where a
	 * `GridRangeConfig` template parameter is required.
	 *
	 * The explicit specialization of VonNeumannRangePerimeter corresponding to
	 * `GridConfig`, i.e. `VonNeumannRangePerimeter<GridConfig>` is
	 * automatically selected: the type is ill-formed if no such specialization
	 * exists.
	 */
	template<typename GridConfig>
	using VonNeumannRangeConfig = GridRangeConfig<ManhattanDistance, VonNeumannRangePerimeter<GridConfig>>;

	/**
	 * VonNeumannRangePerimeter specialization corresponding to a
	 * VonNeumannRange used on a VonNeumannGrid.
	 */
	template<>
		struct VonNeumannRangePerimeter<VonNeumannGrid> {
			/**
			 * Returns the _perimeter_ of the specified VonNeumann `range` on a
			 * VonNeumannGrid.
			 *
			 * In this particular case, the perimeter corresponds to `(0,
			 * range.getSize())`. See GridRangeConfig::Perimeter for more
			 * information about the definition of a "perimeter".
			 *
			 * @param range VonNeumann range for which the perimeter must be
			 * computed
			 */
			DiscretePoint operator()(const GridRange<VonNeumannGrid, VonNeumannRangeConfig<VonNeumannGrid>>& range) const {
				return {0, range.getSize()};
			}
		};

	// MooreGrid case: not available yet
	/*
	 *template<>
	 *    struct VonNeumannRangeConfig::Perimeter<MooreGrid> {
	 *        DiscretePoint operator()(const DiscreteCoordinate& range_size) const {
	 *            return {0, range_size};
	 *        }
	 *    };
	 */
	
	/**
	 * GridRange specialization defining variable size ranges with a VonNeumann
	 * shape.
	 *
	 * Formally, considering a VonNeumannRange `range` centered on `p1`, the range is
	 * constituted by any point of the Grid `p` such that `ManhattanDistance()(p1, p) <=
	 * range.getSize()`.
	 *
	 * Notice that this is completely independent from the underlying shape of
	 * the grid, defined by `GridConfig`.
	 *
	 * @see ManhattanDistance
	 * @see MooreRange
	 * @see GridConfig
	 */
	template<typename GridConfig>
		using VonNeumannRange = GridRange<GridConfig, VonNeumannRangeConfig<GridConfig>>;

}}
#endif
