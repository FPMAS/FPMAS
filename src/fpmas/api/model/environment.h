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

	class Cell : public virtual Agent {
		public:
			virtual std::vector<Cell*> neighborhood() = 0;

			virtual void handleNewLocation() = 0;
			virtual void handleMove() = 0;
			virtual void handlePerceive() = 0;

			virtual void updatePerceptions() = 0;
	};

	template<typename CellType>
	class Range {
		public:
			virtual unsigned int size() const = 0;
			virtual bool contains(CellType* root, CellType* cell) const = 0;
	};

	template<typename CellType>
	class SpatialAgent : public virtual Agent {
		protected:
			virtual void moveTo(CellType*) = 0;
			virtual void moveTo(api::graph::DistributedId id) = 0;
			virtual CellType* locationCell() const = 0;

		public:
			virtual void initLocation(CellType*) = 0;
			virtual api::graph::DistributedId locationId() const = 0;
			virtual const Range<CellType>& mobilityRange() const = 0;
			virtual const Range<CellType>& perceptionRange() const = 0;

			virtual void handleNewMove() = 0;
			virtual void handleNewPerceive() = 0;
	};

	template<typename CellType>
	class Environment {
		public:
			virtual void add(SpatialAgent<CellType>* agent) = 0;
			virtual void add(CellType* cell) = 0;
			virtual std::vector<CellType*> localCells() = 0;

			virtual api::scheduler::JobList initLocationAlgorithm(
					unsigned int max_perception_range,
					unsigned int max_mobility_range) = 0;
			virtual api::scheduler::JobList distributedMoveAlgorithm(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) = 0;

	};

}}}
#endif
