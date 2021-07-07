#ifndef FPMAS_MOORE_RANGE_H
#define FPMAS_MOORE_RANGE_H

#include "moore.h"
#include "von_neumann.h"

namespace fpmas { namespace model {
	/**
	 * MooreRange perimeter function object.
	 *
	 * **Must** be explicitly specialized for each available GridConfig.
	 *
	 * See GridRangeConfig::Perimeter for more information about what a
	 * "perimeter" is.
	 */
	template<typename GridConfig>
		struct MooreRangePerimeter {};

	/**
	 * Moore GridRangeConfig specialization, that might be used where a
	 * `GridRangeConfig` template parameter is required.
	 *
	 * The explicit specialization of MooreRangePerimeter corresponding to
	 * `GridConfig`, i.e. `MooreRangePerimeter<GridConfig>` is
	 * automatically selected: the type is ill-formed if no such specialization
	 * exists.
	 */
	template<typename GridConfig>
	using MooreRangeConfig = GridRangeConfig<ChebyshevDistance, MooreRangePerimeter<GridConfig>>;
	
	/**
	 * MooreRangePerimeter specialization corresponding to a
	 * MooreRange used on a VonNeumannGrid.
	 */
	template<typename CellType>
		struct MooreRangePerimeter<VonNeumannGrid<CellType>> {
			/**
			 * Returns the _perimeter_ of the specified Moore `range` on a
			 * VonNeumannGrid.
			 *
			 * In this particular case, the perimeter corresponds to
			 * `(range.getSize(), range.getSize())`. Indeed, this point
			 * maximizes the distance from the range's origin on a VonNeumann
			 * grid.
			 *
			 * See GridRangeConfig::Perimeter for more information about the
			 * definition of a "perimeter".
			 *
			 * @param range VonNeumann range for which the perimeter must be
			 * computed
			 */
			DiscretePoint operator()(
					const GridRange<VonNeumannGrid<CellType>,
					MooreRangeConfig<VonNeumannGrid<CellType>>>& range) const {
				return {range.getSize(), range.getSize()};
			}
		};

	/**
	 * MooreRangePerimeter specialization corresponding to a
	 * MooreRange used on a VonNeumannGrid.
	 */
	template<typename CellType>
		struct MooreRangePerimeter<MooreGrid<CellType>> {
			/**
			 * Returns the _perimeter_ of the specified Moore `range` on a
			 * MooreGrid.
			 *
			 * In this particular case, the perimeter corresponds to `(0,
			 * range.getSize())`. Indeed, this point maximizes the distance
			 * from the range's origin on a Moore grid.
			 *
			 * See GridRangeConfig::Perimeter for more information about the
			 * definition of a "perimeter".
			 *
			 * @param range Moore range for which the perimeter must be
			 * computed
			 */
			DiscretePoint operator()(
					const GridRange<MooreGrid<CellType>,
					MooreRangeConfig<MooreGrid<CellType>>>& range) const {
				return {0, range.getSize()};
			}
		};

	/**
	 * GridRange specialization defining variable size ranges with a Moore
	 * shape.
	 *
	 * Formally, considering a VonNeumannRange `range` centered on `p1`, the range is
	 * constituted by any point of the Grid `p` such that `ChebyshevDistance()(p1, p) <=
	 * range.getSize()`.
	 *
	 * Notice that this is completely independent from the underlying shape of
	 * the grid, defined by `GridConfig`.
	 *
	 * @see ChebyshevDistance
	 * @see VonNeumannRange
	 * @see GridConfig
	 */
	template<typename GridConfig>
		using MooreRange = GridRange<GridConfig, MooreRangeConfig<GridConfig>>;


}}
#endif
