#ifndef FPMAS_GRID_API_H
#define FPMAS_GRID_API_H

#include "environment.h"

namespace fpmas { namespace api { namespace model {

	typedef long DiscreteCoordinate;

	struct DiscretePoint {
		DiscreteCoordinate x;
		DiscreteCoordinate y;

		DiscretePoint() {}
		DiscretePoint(DiscreteCoordinate x, DiscreteCoordinate y)
			: x(x), y(y) {}
	};

	class GridCell : public virtual Cell {
		public:
			virtual DiscretePoint location() = 0;
	};

	class GridAgent : public virtual SpatialAgent<GridCell> {
		public:
			virtual void moveToPoint(DiscretePoint point) = 0;
			virtual DiscretePoint currentLocation() = 0;
	};

	template<typename CellType>
	class EnvironmentBuilder {
		public:
			virtual void build(Environment<CellType>& environment) = 0;
	};

}}}
#endif
