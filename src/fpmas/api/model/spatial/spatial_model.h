#ifndef FPMAS_ENVIRONMENT_API_H
#define FPMAS_ENVIRONMENT_API_H

#include "../model.h"

namespace fpmas { namespace api { namespace model {

	enum SpatialModelLayers : fpmas::api::graph::LayerId {
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

	template<typename CellType> class Range { public: virtual bool
		contains(CellType* root, CellType* cell) const = 0;
			/** Returns the radius of this range, i.e. a value greater or equal
			 * to the maximum shortest path length between `origin` and any
			 * cell contained in this range, considering `origin` as the
			 * location of the object to which this range is associated.
			 *
			 * Notice that such path length must be understood as a "graph
			 * distance", and not confused with any other type of distance that
			 * might be associated to `CellType`.
			 *
			 * It represents the maximum count of edges on a shortest path from
			 * `origin` to any point in the range in the current Cell network.
			 * In consequence, implementations will probably depend on the
			 * underlying environment implementation on which this range is
			 * used (Grid, generic graph, geographic graph...).
			 *
			 * Here is a trivial example. Consider that the current environment
			 * (or "Cell network") is a simple chain of cells, with a distance
			 * of 1km between each cell. The underlying graph might be
			 * represented as follow: 1 -- 2 -- 3 -- 4 -- 5 -- 6 -- ...
			 *
			 * Now, let's implement a Range that represents the fact that a
			 * SpatialAgent can move 2km to its left or to its right.
			 * Considering the definition above, the radius returned by this
			 * method will be 2, since that, considering the current
			 * environment, a SpatialAgent associated to this Range will be
			 * able to move at most two cells to its left or to its right,
			 * whatever its current position is (so, in this case, there is no
			 * need to use the `origin` parameter).
			 */
			virtual std::size_t radius(CellType* origin) const = 0;

			virtual ~Range() {}
	};

	class SpatialAgentBehavior {
		public:
			virtual void handleNewMove() = 0;
			virtual void handleNewPerceive() = 0;

			virtual ~SpatialAgentBehavior() {}
	};

	// Template free agent base
	template<typename CellType>
	class SpatialAgent : public virtual Agent, public SpatialAgentBehavior {
		public:
			typedef CellType Cell;

			virtual void moveTo(api::graph::DistributedId id) = 0;
			virtual api::graph::DistributedId locationId() const = 0;
			virtual void initLocation(Cell*) = 0;
			virtual const Range<CellType>& mobilityRange() const = 0;
			virtual const Range<CellType>& perceptionRange() const = 0;
			virtual CellType* locationCell() const = 0;

		protected:
			virtual void moveTo(Cell*) = 0;
	};

	template<typename CellType>
		class EndCondition {
			public:
				virtual void init(
						api::communication::MpiCommunicator& comm,
						std::vector<api::model::SpatialAgent<CellType>*> agents,
						std::vector<CellType*> cells) = 0;
				virtual void step() = 0;
				virtual bool end() = 0;

				virtual ~EndCondition() {};
		};

	template<typename CellType> class SpatialModel;
	template<typename CellType>
	class DistributedMoveAlgorithm {
		public:
			virtual api::scheduler::JobList jobs(
					SpatialModel<CellType>& model,
					std::vector<api::model::SpatialAgent<CellType>*> agents,
					std::vector<CellType*> cells) = 0;

			virtual ~DistributedMoveAlgorithm() {}
	};

	template<typename CellType>
	class SpatialModel : public virtual Model {
		public:
			virtual void add(CellType* cell) = 0;
			virtual std::vector<CellType*> cells() = 0;
			virtual AgentGroup& buildMoveGroup(GroupId id, const Behavior& behavior) = 0;

			virtual DistributedMoveAlgorithm<CellType>& distributedMoveAlgorithm() = 0;

			virtual ~SpatialModel() {}
	};

	template<typename CellType>
	class SpatialModelBuilder {
		public:
			virtual std::vector<CellType*> build(SpatialModel<CellType>& spatial_model) = 0;

			virtual ~SpatialModelBuilder() {}
	};

	template<typename CellType>
	class SpatialAgentFactory {
		public:
			virtual SpatialAgent<CellType>* build() = 0;

			virtual ~SpatialAgentFactory() {}
	};

	template<typename CellType>
	class SpatialAgentMapping {
		public:
			virtual std::size_t countAt(CellType*) = 0;

			virtual ~SpatialAgentMapping() {}
	};

	template<typename CellType>
	class SpatialAgentBuilder {
		public:
			virtual void build(
					SpatialModel<CellType>& model,
					GroupList groups,
					SpatialAgentFactory<CellType>& factory,
					SpatialAgentMapping<CellType>& agent_counts
					) = 0;

			virtual ~SpatialAgentBuilder() {}
	};

}}}
#endif
