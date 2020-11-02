#ifndef FPMAS_ENVIRONMENT_API_H
#define FPMAS_ENVIRONMENT_API_H

#include "model.h"

namespace fpmas { namespace api { namespace model {

	enum EnvironmentLayers : fpmas::api::graph::LayerId {
		NEIGHBOR_CELL = -1,
		LOCATION = -2,
		MOVE = -3,
		PERCEIVE = -4,
		PERCEPTION = -5,
		NEW_LOCATION = -6,
		NEW_MOVE = -7,
		NEW_PERCEIVE = -8
	};

	class LocatedAgent;

	class Cell : public virtual Agent {
		protected:
			virtual void updateLocation(Neighbor<LocatedAgent>& agent) = 0;
			virtual void growMobilityRange(LocatedAgent* agent) = 0;
			virtual void growPerceptionRange(LocatedAgent* agent) = 0;

		public:
			virtual void updateRanges() = 0;
			virtual void updatePerceptions() = 0;
	};

	class LocatedAgent : public virtual Agent {
		public:
			virtual void moveToCell(Cell*) = 0;

			virtual Cell* location() = 0;
			virtual unsigned int mobilityRange() = 0;
			virtual bool isInMobilityRange(Cell*) = 0;
			virtual unsigned int perceptionRange() = 0;
			virtual bool isInPerceptionRange(Cell*) = 0;
	};

	class Environment {
		public:
			virtual api::scheduler::JobList jobs(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) = 0;

			virtual AgentGroup& cells() = 0;

	};


	typedef unsigned long DiscreteCoordinate;

	struct DiscretePoint {
		DiscreteCoordinate x;
		DiscreteCoordinate y;
	};


	class GridCell : public Cell {
		public:
			virtual DiscretePoint location() = 0;
	};

	class GridAgent : public LocatedAgent {
		public:
			virtual GridCell* location() override = 0;

			virtual void moveToPoint(DiscretePoint point) = 0;
	};

}}}
#endif
