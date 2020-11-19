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

	class CellBehavior {
		public:
			virtual void handleNewLocation() = 0;
			virtual void handleMove() = 0;
			virtual void handlePerceive() = 0;

			virtual void updatePerceptions() = 0;

			virtual ~CellBehavior() {}
	};

	class Cell : public virtual Agent, public CellBehavior {
		public:
			virtual std::vector<Cell*> neighborhood() = 0;

			virtual ~Cell() {}
	};

	template<typename CellType>
	class Range {
		public:
			virtual unsigned int size() const = 0;
			virtual bool contains(CellType* root, CellType* cell) const = 0;

			virtual ~Range() {}
	};

	class SpatialAgentBehavior {
		public:
			virtual void handleNewMove() = 0;
			virtual void handleNewPerceive() = 0;

			virtual ~SpatialAgentBehavior() {}
	};

	class SpatialAgentBase : public virtual Agent, public SpatialAgentBehavior {

	};

	class SpatialAgent : public SpatialAgentBase {
		protected:
			virtual void moveTo(Cell*) = 0;
			virtual void moveTo(api::graph::DistributedId id) = 0;
			virtual Cell* locationCell() const = 0;

		public:
			virtual void initLocation(Cell*) = 0;
			virtual api::graph::DistributedId locationId() const = 0;
	};

	class Environment {
		public:
			virtual void add(SpatialAgentBase* agent) = 0;
			virtual void add(Cell* cell) = 0;
			virtual std::vector<Cell*> cells() = 0;

			virtual api::scheduler::JobList initLocationAlgorithm(
					unsigned int max_perception_range,
					unsigned int max_mobility_range) = 0;
			virtual api::scheduler::JobList distributedMoveAlgorithm(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) = 0;

			virtual ~Environment() {}
	};

	class EnvironmentBuilder {
		public:
			virtual void build(
					Model& model,
					Environment& environment) = 0;

			virtual ~EnvironmentBuilder() {}
	};

	class SpatialAgentFactory {
		public:
			virtual SpatialAgent* build() = 0;

			virtual ~SpatialAgentFactory() {}
	};

	class SpatialAgentMapping {
		public:
			virtual std::size_t countAt(Cell*) = 0;

			virtual ~SpatialAgentMapping() {}
	};

	class SpatialAgentBuilder {
		public:
			virtual void build(
					SpatialAgentFactory& factory,
					SpatialAgentMapping& agent_counts,
					std::vector<Cell*> local_cells) = 0;

			virtual ~SpatialAgentBuilder() {}
	};

}}}
#endif
