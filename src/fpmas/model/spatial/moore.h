#ifndef FPMAS_MOORE_H
#define FPMAS_MOORE_H

#include "von_neumann.h"

namespace fpmas { namespace model {

	class ChebyshevDistance {
		public:
			DiscreteCoordinate operator()(
					const DiscretePoint& p1,
					const DiscretePoint& p2) const {
				return std::max(std::abs(p2.x - p1.x), std::abs(p2.y - p1.y));
			}
	};

	/**
	 * MooreRange perimeter function object.
	 *
	 * **Must** be explicitly specialized for each available GridConfig.
	 */
	template<typename GridConfig>
		struct MooreRangePerimeter {};

	template<typename GridConfig>
	using MooreRangeConfig = GridRangeConfig<ChebyshevDistance, MooreRangePerimeter<GridConfig>>;
	
	template<>
		struct MooreRangePerimeter<VonNeumannGrid> {
			DiscretePoint operator()(const GridRange<VonNeumannGrid, MooreRangeConfig<VonNeumannGrid>>& range) const {
				return {range.getSize(), range.getSize()};
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

	template<typename GridConfig>
		using MooreRange = GridRange<GridConfig, MooreRangeConfig<GridConfig>>;

}}
#endif
