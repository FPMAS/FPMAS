#ifndef FPMAS_VON_NEUMANN_RANGE_H
#define FPMAS_VON_NEUMANN_RANGE_H

#include "moore.h"
#include "von_neumann.h"

namespace fpmas { namespace model {

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
	template<typename CellType>
		struct VonNeumannRangePerimeter<VonNeumannGrid<CellType>> {
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
			DiscretePoint operator()(const GridRange<VonNeumannGrid<CellType>, VonNeumannRangeConfig<VonNeumannGrid<CellType>>>& range) const {
				return {0, range.getSize()};
			}
		};

	/**
	 * VonNeumannRangePerimeter specialization corresponding to a
	 * VonNeumannRange used on a MooreGrid.
	 */
	template<typename CellType>
		struct VonNeumannRangePerimeter<MooreGrid<CellType>> {
			/**
			 * Returns the _perimeter_ of the specified VonNeumann `range` on a
			 * MooreGrid.
			 *
			 * In this particular case, the perimeter corresponds to `(0,
			 * range.getSize())`. See GridRangeConfig::Perimeter for more
			 * information about the definition of a "perimeter".
			 *
			 * @param range VonNeumann range for which the perimeter must be
			 * computed
			 */
			DiscretePoint operator()(
					const GridRange<MooreGrid<CellType>,
					VonNeumannRangeConfig<MooreGrid<CellType>>>& range) const {
				return {0, range.getSize()};
			}
		};
	
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
