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
			virtual DiscretePoint location() const = 0;
	};

	class GridAgent : public virtual SpatialAgent<GridCell> {
		protected:
			using SpatialAgent<GridCell>::moveTo;

			virtual void moveTo(DiscretePoint point) = 0;
		public:
			virtual DiscretePoint locationPoint() const = 0;
	};

	class GridCellFactory {
		public:
			virtual GridCell* build(DiscretePoint location) = 0;

			virtual ~GridCellFactory() {}
	};

	class GridAgentFactory {
		public:
			virtual GridAgent* build() = 0;

			virtual ~GridAgentFactory() {}
	};

	class GridAgentMapping {
		public:
			virtual std::size_t countAt(DiscretePoint) = 0;

			virtual ~GridAgentMapping() {}
	};
	class DistributedGridAgentBuilder {
		public:
			virtual void build(
				GridAgentFactory& factory,
				GridAgentMapping& agent_counts,
				std::vector<DiscretePoint> local_points) = 0;

			virtual ~DistributedGridAgentBuilder() {}
	};
}}}

namespace fpmas {
	std::string to_string(const api::model::DiscretePoint& point);

	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const DiscretePoint& point);
	}}
}
#endif
