#ifndef FPMAS_ENVIRONMENT_API_H
#define FPMAS_ENVIRONMENT_API_H

/** \file src/fpmas/api/model/spatial/spatial_model.h
 * Spatial models API.
 */

#include "../model.h"

namespace fpmas { namespace api { namespace model {

	/**
	 * Predefined graph layers reserved by FPMAS to implement the
	 * DistributedMoveAlgorithm.
	 * Does not conflict with the FPMAS_DEFINE_GROUPS macro.
	 */
	enum SpatialModelLayers : fpmas::api::graph::LayerId {
		CELL_SUCCESSOR = -1,
		LOCATION = -2,
		MOVE = -3,
		PERCEIVE = -4,
		PERCEPTION = -5,
		NEW_LOCATION = -6,
		NEW_MOVE = -7,
		NEW_PERCEIVE = -8
	};

	/**
	 * Predefined Cell behaviors specifically used in the context of the
	 * DistributedMoveAlgorithm.
	 */
	class CellBehavior {
		public:
			/**
			 * Implementation free initialization method, called on each Cell
			 * when the DistributedMoveAlgorithm is run, before any other
			 * behavior is executed.
			 */
			virtual void init() = 0;
			/**
			 * Handles links on the NEW_LOCATION layer.
			 *
			 * NEW_LOCATION links are replaced by LOCATION links, and the
			 * "range growing" algorithm is initialized, linking each
			 * NEW_LOCATION edges' sources to this cell's successors on the
			 * NEW_MOVE and NEW_PERCEIVE layers.
			 */
			virtual void handleNewLocation() = 0;
			/**
			 * Handles links on the MOVE layer.
			 *
			 * Expands the current mobility range of MOVE edges' sources, if
			 * they have not been handled yet by this Cell. Expansion is
			 * performed linking each source to this cell's successors on the
			 * NEW_MOVE layer.
			 */
			virtual void handleMove() = 0;
			/**
			 * Handles links on the PERCEIVE layer.
			 *
			 * Expands the current perception range of PERCEIVE edges' sources, if
			 * they have not been handled yet by this Cell. Expansion is
			 * performed linking each source to this cell's successors on the
			 * NEW_PERCEIVE layer.
			 */
			virtual void handlePerceive() = 0;

			/**
			 * Updates the perceptions of agents "perceiving" this Cell.
			 *
			 * More precisely, each agent connected to this Cell on the
			 * PERCEIVE layer is linked to all agents located in this Cell,
			 * (i.e. connected on the LOCATION layer) on the PERCEPTION layer.
			 */
			virtual void updatePerceptions(AgentGroup& group) = 0;

			virtual ~CellBehavior() {}
	};

	/**
	 * Cell API.
	 *
	 * \Cells represent nodes on which \SpatialAgents can be located. Moreover,
	 * \SpatialAgents can move to other \Cells and perceive objects located on
	 * others, according to their mobility and perception ranges.
	 *
	 * Such operation occur in the "Cell Network", i.e. the subgraph that can
	 * be described using the Cell::successors() methods.
	 */
	class Cell : public virtual Agent, public CellBehavior {
		public:
			/**
			 * Returns successors of this Cell.
			 *
			 * Successors are targets of outgoing edges from this Cell on the
			 * CELL_SUCCESSOR layer.
			 *
			 * This method is assumed to be called on a LOCAL Cell. The
			 * returned list might be incomplete otherwise.
			 *
			 * @return successors of this Cell
			 */
			virtual std::vector<Cell*> successors() = 0;

			virtual ~Cell() {}
	};

	/**
	 * Range API.
	 *
	 * Ranges might be associated to \SpatialAgents to build their mobility and
	 * perception fields.
	 *
	 * Ranges are assumed to be continuous: for any given `root`, if
	 * `this->contains(root, cell)`, the current Cell network must contain a
	 * path from `root` to `cell` such that `this->contains(root, cell_path)`
	 * is `true` for any `cell_path` on the path. This is required for the
	 * DistributedMoveAlgorithm to work properly.
	 */
	template<typename CellType>
		class Range {
			public:
				/**
				 * Returns `true` if and only if `cell` is contained in this
				 * range, considering that the range uses `root` as its origin.
				 *
				 * In practice, `root` is usually the location of the
				 * SpatialAgent to which this range is associated.
				 *
				 * Notice that `root` and `cell` can be \DISTANT. In
				 * consequence, it is not safe to use `root`'s or `cell`'s
				 * neighbors in this method implementation.
				 *
				 * @param root origin of the range
				 * @param cell cell to check
				 */
				virtual bool contains(CellType* root, CellType* cell) const = 0;

				/**
				 * Returns the radius of this range, i.e. a value greater or
				 * equal to the maximum shortest path length between `origin`
				 * and any cell contained in this range, considering `origin`
				 * as the location of the object to which this range is
				 * associated.
				 *
				 * Notice that such path length must be understood as a "graph
				 * distance", and not confused with any other type of distance
				 * that might be associated to `CellType`.
				 *
				 * It represents the maximum count of edges on a shortest path
				 * from `origin` to any point in the range in the current Cell
				 * network.  In consequence, implementations will probably
				 * depend on the underlying environment implementation on which
				 * this range is used (Grid, generic graph, geographic
				 * graph...).
				 *
				 * Here is a trivial example. Consider that the current
				 * environment (or "Cell network") is a simple chain of cells,
				 * with a distance of 1km between each cell. The underlying
				 * graph might be represented as follow: 1 -- 2 -- 3 -- 4 -- 5
				 * -- 6 -- ...
				 *
				 * Now, let's implement a Range that represents the fact that a
				 * SpatialAgent can move 2km to its left or to its right.
				 * Considering the definition above, the radius returned by
				 * this method will be 2, since in the current environment a
				 * SpatialAgent associated to this Range will be able to move
				 * at most two cells to its left or to its right, whatever its
				 * current position is (so, in this case, there is no need to
				 * use the `origin` parameter).
				 *
				 * @param origin origin cell of the range (typically, the
				 * location of the agent associated to the range)
				 * @return range radius
				 */
				virtual std::size_t radius(CellType* origin) const = 0;

				virtual ~Range() {}
		};

	/**
	 * Predefined SpatialAgent behaviors specifically used in the context of the
	 * DistributedMoveAlgorithm.
	 */
	class SpatialAgentBehavior {
		public:
			/**
			 * Handles links on the NEW_MOVE layer.
			 *
			 * Crops the current mobility field according to the current
			 * mobility range of the agent.
			 *
			 * For each outgoing edge on the NEW_MOVE layer:
			 * - if `agent->mobilityRange().contains(agent->locationCell(), edge->getTargetNode())`,
			 *   edge is replaced by an edge on the MOVE layer.
			 * - else, the edge is just unlinked.
			 */
			virtual void handleNewMove() = 0;

			/**
			 * Handles links on the NEW_PERCEIVE layer.
			 *
			 * Crops the current perception field according to the current
			 * perception range of the agent.
			 *
			 * For each outgoing edge on the NEW_PERCEIVE layer:
			 * - if `agent->perceptionRange().contains(agent->locationCell(), edge->getTargetNode())`,
			 *   edge is replaced by an edge on the PERCEIVE layer.
			 * - else, the edge is just unlinked.
			 */
			virtual void handleNewPerceive() = 0;

			virtual ~SpatialAgentBehavior() {}
	};

	/**
	 * SpatialAgent API.
	 *
	 * In addition to the classical Agent properties, \SpatialAgents can be
	 * located on a Cell network, move and perceive objects within it.
	 *
	 * The _mobility field_ of a SpatialAgent is defined as the set of outgoing
	 * neighbors of type Cell on the `MOVE` layer.
	 *
	 * The _perceptions_ of a SpatialAgent are defined as elements of the set
	 * of outgoind neighbors of type Agent on the `PERCEPTION` layer.
	 *
	 * Helper functions might be defined in implementing classes to access
	 * those elements.
	 *
	 * The specified `CellType` must extend Cell.
	 */
	template<typename CellType>
	class SpatialAgent : public virtual Agent, public SpatialAgentBehavior {
		public:
			static_assert(std::is_base_of<api::model::Cell, CellType>::value,
					"CellType must extend api::model::Cell");
			/**
			 * Type of Cell on which the SpatialAgent is moving.
			 */
			typedef CellType Cell;

			
			/**
			 * Returns the `id` of the location Cell of this SpatialAgent.
			 *
			 * This id can be used to easily move to the location of DISTANT
			 * perceived SpatialAgent.
			 *
			 * For example, let's consider a SpatialAgent `A`, that is LOCAL.
			 * It can perceive a DISTANT agent `B`, located on a Cell `C`, that
			 * is also DISTANT. We also assume `C` is currently in the
			 * _mobility field_ of `A`. Considering the properties of an FPMAS
			 * DistributedGraph, `B` is guaranteed to be represented on the
			 * current process, since it's linked to `A` on the `PERCEPTION`
			 * layer, and so does `C` on the `MOVE` layer. However, since `B`
			 * and `C` are distant, the `LOCATION` link from `B` to `C` is not
			 * expected to be presented on the current process, so calling
			 * `B->locationCell()` will produce an unexpected behavior.
			 * However, `A` can still move to the current location of `B` using
			 * the locationId() method:
			 * ```cpp
			 * A->moveTo(B->locationId());
			 * ```
			 *
			 * Notice that this is valid whatever the current
			 * SynchronizationMode is. Indeed, `moveTo` operations are only
			 * committed at the next DistributedMoveAlgorithm execution, that
			 * includes a Graph synchronization process. So, in any case, even
			 * if `B` moves during its execution on an other process while
			 * `A->moveTo()` is executed on the current process, its new
			 * location will effectively be committed only when all
			 * \SpatialAgents are synchronized, so it is still consistent to
			 * assume that, during this epoch, `B` is still located at
			 * `B->locationId()`. However, when the DistributedMoveAlgorithm
			 * will be applied, `A` will finally end on a different location
			 * than `B`.
			 *
			 * @return id of the current location
			 */
			virtual DistributedId locationId() const = 0;

			/**
			 * Returns the current location of this SpatialAgent.
			 *
			 * The location of a SpatialAgent is defined as the target of the
			 * only outgoing edge connected to this SpatialAgent on the
			 * `LOCATION` layer.
			 *
			 * Contrary to locationId(), this method produces an unexpected
			 * result when called on a DISTANT SpatialAgent.
			 *
			 * @return current location
			 */
			virtual CellType* locationCell() const = 0;

			/**
			 * Initializes the location of this SpatialAgent.
			 *
			 * Contrary to `moveTo`, this method is public and does not assume
			 * that old location, mobility field and perceptions must be
			 * unlinked. If the SpatialAgent location was already defined
			 * (using initLocation() or moveTo()), the behavior of this method
			 * is undefined. However, notice that the location of the
			 * SpatialAgent can also be initialized internally using a moveTo()
			 * operation.
			 *
			 * The DistributedMoveAlgorithm must still be applied to commit the
			 * initial location, as with regular moveTo() operations.
			 *
			 * @param cell initial location of the SpatialAgent
			 */
			virtual void initLocation(Cell* cell) = 0;

			/**
			 * Returns the current mobility range of this SpatialAgent.
			 *
			 * @return mobility range
			 */
			virtual const Range<CellType>& mobilityRange() const = 0;

			/**
			 * Returns the current perception range of this SpatialAgent.
			 *
			 * @return perception range
			 */
			virtual const Range<CellType>& perceptionRange() const = 0;

		protected:
			/**
			 * Moves to the Cell with the provided `id`.
			 *
			 * If the `id` does not correspond to any Cell in the current
			 * _mobility field_, an OutOfMobilityFieldException is thrown.
			 *
			 * If a corresponding Cell is found, the move operation is
			 * performed as described in moveTo(Cell*).
			 *
			 * @param id Cell id
			 * @throw OutOfMobilityFieldException
			 */
			virtual void moveTo(DistributedId id) = 0;

			/**
			 * Moves to the input Cell.
			 *
			 * The input Cell is not required to be contained in the current
			 * _mobility field_ of this SpatialAgent, what allows:
			 * 1. To initialize the location of this SpatialAgent (since in this
			 * case, the _mobility field_ is still empty)
			 * 2. A SpatialAgent to move an other DISTANT SpatialAgent.
			 *
			 * The execution of a DistributedMoveAlgorithm is required to
			 * commit the operation and update mobility fields and perceptions.
			 * However, this can be handled automatically on specific
			 * AgentGroups: see SpatialModel::buildMoveGroup().
			 *
			 * More precisely, the current location, if defined, is unlinked,
			 * and so does the mobility field and perceptions, and a link from
			 * this SpatialAgent to `cell` is created on the `NEW_LOCATION`
			 * layer. In consequence, the behavior of locationCell() is
			 * undefined until a DistributedMoveAlgorithm is executed.
			 *
			 * Notice that performing an other moveTo() operation before the
			 * previous is committed produces an undefined behavior.
			 *
			 * @param cell cell to which this SpatialAgent should move.
			 */
			virtual void moveTo(Cell* cell) = 0;
	};

	/**
	 * Generic EndCondition that can be used by a DistributedMoveAlgorithm.
	 */
	template<typename CellType>
		class EndCondition {
			public:
				/**
				 * Initializes the end condition.
				 *
				 * Since this method is synchronously called by the
				 * DistributedMoveAlgorithm on each process, it is possible to
				 * perform collective communications using the provided
				 * MpiCommunicator.
				 *
				 * The provided `agents` list contains all the LOCAL
				 * \SpatialAgents on which the DistributedMoveAlgorithm will be
				 * applied.
				 *
				 * The `cells` contains LOCAL cells of the current Cell
				 * network, within which agents are moving.
				 *
				 * @param comm MPI communicator
				 * @param agents LOCAL agents on which the
				 * DistributedMoveAlgorithm is applied
				 * @param cells LOCAL cells of the current Cell network
				 */
				virtual void init(
						api::communication::MpiCommunicator& comm,
						std::vector<api::model::SpatialAgent<CellType>*> agents,
						std::vector<CellType*> cells) = 0;

				/**
				 * Advances the end condition.
				 */
				virtual void step() = 0;
				/**
				 * Returns true if and only if the end condition has been
				 * reached.
				 *
				 * @return true iff the end condition has been reached
				 */
				virtual bool end() = 0;

				virtual ~EndCondition() {};
		};

	template<typename CellType> class SpatialModel;


	/**
	 * DistributedMoveAlgorithm API.
	 */
	template<typename CellType>
	class DistributedMoveAlgorithm {
		public:
			/**
			 * Returns an executable \JobList that can be used to update the
			 * location, mobility field and perceptions of all `agents`.
			 *
			 * The returned job list can be scheduled right after the execution
			 * of `agents`' behaviors that include SpatialAgent::moveTo()
			 * operations, in order to commit them. Notice that a graph
			 * synchronization is required between those operations and the
			 * execution of jobs returned by this method.
			 *
			 * All those operations can be handled automatically using
			 * dedicated \AgentGroups: see SpatialModel::buildMoveGroup().
			 *
			 * @param model current SpatialModel
			 * @param agents LOCAL agents to move
			 * @param cells LOCAL cells of the current Cell network, within
			 * which agents are moving
			 */
			virtual api::scheduler::JobList jobs() const = 0;

			virtual ~DistributedMoveAlgorithm() {}
	};

	/**
	 * \AgentGroup designed to implicitly include a \DistributedMoveAlgorithm
	 * in the JobList returned by jobs().
	 */
	template<typename CellType>
		class MoveAgentGroup : public virtual AgentGroup {
			public:
				/**
				 * Returns the internal distributedMoveAlgorithm() associated
				 * to this group.
				 *
				 * The execution of distributedMoveAlgorithm().jobs() apply the
				 * DistributedMoveAlgorithm to all agents currently contained
				 * in the group.
				 *
				 * The JobList returned by jobs() corresponds to
				 * ```cpp
				 * {this->agentExecutionJob(), this->distributedMoveAlgorithm().jobs()}
				 * ```
				 *
				 * @return reference to internal distributed move algorithm instance
				 */
				virtual DistributedMoveAlgorithm<CellType>& distributedMoveAlgorithm() = 0;
		};

	/**
	 * Model API extension to handle \SpatialAgents.
	 */
	template<typename CellType>
	class SpatialModel : public virtual Model {
		public:
			/**
			 * Adds a Cell to this model.
			 *
			 * \Cells added to the model constitute the "Cell network", on
			 * which \SpatialAgents, added to groups created with
			 * buildMoveGroup(), will be able to move.
			 *
			 * @param cell cell to add to the model
			 */
			virtual void add(CellType* cell) = 0;

			/**
			 * Returns the list of LOCAL cells on the current process.
			 *
			 * Such \Cells have necessarily been added to the model using
			 * add(), but might have migrate across processes during
			 * load-balancing operations.
			 *
			 * @return local \Cells list
			 */
			virtual std::vector<CellType*> cells() = 0;

			/**
			 * Returns the AgentGroup containing all \Cells in the current
			 * SpatialModel.
			 *
			 * @return cell group
			 */
			virtual AgentGroup& cellGroup() = 0;

			/**
			 * Builds an agent group that automatically includes the
			 * DistributedMoveAlgorithm execution.
			 *
			 * The specified `behavior` is assumed to include some
			 * SpatialAgent::moveTo() operations, even if it's not required.
			 * Contrary to the Model::buildGroup() method, the returned group
			 * is such that in addition to the execution of the `behavior` for
			 * each agent of the group, the AgentGroup::jobs() list also
			 * includes the DistributedMoveAlgorithm::jobs() list, built
			 * properly from cells() in the model and agents contained in the
			 * AgentGroup.
			 *
			 * @param id unique group id
			 * @param behavior agent behavior
			 * @return an AgentGroup such that AgentGroup::jobs() is suitable
			 * for the execution and commitment of SpatialAgent::moveTo()
			 * operations
			 */
			virtual MoveAgentGroup<CellType>& buildMoveGroup(GroupId id, const Behavior& behavior) = 0;

			virtual ~SpatialModel() {}
	};

	/**
	 * CellNetworkBuilder API.
	 *
	 * The main purpose of a CellNetworkBuilder is to generate some \Cells
	 * according to specific environment properties (for example, a grid with a
	 * specified size or a geographic environment loaded from an OSM file),
	 * and to add it to a predefined SpatialModel in order to build a Cell
	 * network. In consequence, successor relations between \Cells are also
	 * assumed to be built by the CellNetworkBuilder.
	 *
	 * The CellNetworkBuilder is however allowed to instantiate and link any
	 * other type of Agent that might be relevant in the context of the built
	 * environment.
	 *
	 * The construction of the SpatialModel might be distributed, so that the
	 * instantiation of all \Cells are not handled by a single process, but
	 * this is not required.
	 *
	 * The distributed instantiation of user defined
	 * \SpatialAgents on the built Cell network can be handled using a
	 * SpatialAgentBuilder.
	 */
	template<typename CellType>
	class CellNetworkBuilder {
		public:
			/**
			 * Builds a Cell network into the specified `spatial_model`.
			 *
			 * Built \Cells are added to the SpatialModel using the
			 * SpatialModel::add() method.
			 *
			 * The method also perform synchronizations on the underlying
			 * `spatial_model` graph to ensure that all links created are
			 * properly committed and synchronized upon return.
			 *
			 * @param spatial_model spatial model in which cells will be added
			 * @return cells built on the current process
			 */
			virtual std::vector<CellType*> build(
					SpatialModel<CellType>& spatial_model
					) const = 0;

			/**
			 * Same as `build(spatial_model)`, but also adds built
			 * cells to `groups`, in order to assign behaviors to cells.
			 *
			 * @param spatial_model spatial model in which cells will be added
			 * @param groups groups to which built cells will be added
			 * @return cells built on the current process
			 */
			virtual std::vector<CellType*> build(
					SpatialModel<CellType>& spatial_model,
					GroupList groups
					) const = 0;

			virtual ~CellNetworkBuilder() {}
	};

	/**
	 * SpatialAgentFactory API.
	 */
	template<typename CellType>
	class SpatialAgentFactory {
		public:
			/**
			 * Returns a pointer to a dynamically allocated SpatialAgent.
			 *
			 * @return pointer to dynamically allocated SpatialAgent
			 */
			virtual SpatialAgent<CellType>* build() = 0;

			virtual ~SpatialAgentFactory() {}
	};

	/**
	 * The SpatialAgentMapping API is a key feature required for the
	 * distributed SpatialAgent instantiation performed by a
	 * SpatialAgentBuilder.
	 *
	 * The idea of the SpatialAgentBuilder is that not all processes must
	 * instantiate all \SpatialAgents of the simulation, or at least the
	 * instantiation of all \SpatialAgents must not be handled by a single
	 * process.
	 *
	 * However, in accordance with the FPMAS philosophy, we still want to
	 * provide some way to globally initialize a Model, completely abstracting
	 * the underlying Cell network distribution, that might be built in a
	 * distributed way using a CellNetworkBuilder.
	 *
	 * As a counter example, in
	 * [RepastHPC](https://repast.github.io/repast_hpc.html), a determined part
	 * of the environment, represented as a grid, is instantiated on each
	 * process, and the user then provides the number of agents that must be
	 * initialized on each process, i.e. on each part of the environment.
	 * However, this not very flexible, and above all very distribution
	 * dependent. In addition, it requires users to be aware of the underlying
	 * grid distribution to consistently initialize their agents.
	 *
	 *
	 * The SpatialAgentMapping is a proposition to solve those issues in a
	 * generic, flexible and distributed way. The idea is that, instead of
	 * manually instantiating agents and initializing their locations, we can
	 * globally provide rules such as "we want to initialize 1000 preys
	 * uniformly on our distributed grid". Since such rules are assumed to be
	 * computed relatively easily, it is realistic to instantiate them on **all
	 * processes** (for example, loading an external input file). Then, the
	 * SpatialAgentBuilder algorithm can instantiate \SpatialAgents in a
	 * distributed way, only initializing the required number of agents on each
	 * \Cell LOCAL to current process.
	 */
	template<typename CellType>
	class SpatialAgentMapping {
		public:
			/**
			 * Returns the number of \SpatialAgents that must be initialized on
			 * the provided Cell.
			 *
			 * Notice that the agents instantiation and initialization is
			 * **not** performed by this method, but handled by
			 * SpatialAgentBuilder::build().
			 *
			 * @param cell cell on which agents will be initialized
			 */
			virtual std::size_t countAt(CellType* cell) = 0;

			virtual ~SpatialAgentMapping() {}
	};

	/**
	 * SpatialAgentBuilder API.
	 *
	 * The purpose of the SpatialAgentBuilder is to instantiate and initialize
	 * \SpatialAgents in a distributed way, independently of the current Cell
	 * network distribution.
	 *
	 * The `CellType` is the concrete type of Cell used by the SpatialModel.
	 * The `MappingCellType` is the type used by the SpatialAgentMapping.
	 * Those types does not need to be the same. See implementations for
	 * examples.
	 */
	template<typename CellType, typename MappingCellType>
	class SpatialAgentBuilder {
		public:
			/**
			 * Build \SpatialAgents in the specified `model` in a distributed
			 * way.
			 *
			 * More precisely:
			 * 1. For each `cell` in `model.cells()`, `agent_mapping.countAt(cell)`
			 *   \SpatialAgents are allocated using `factory.build()`.
			 * 2. All built agents are added to all the \AgentGroups specified
			 *   in `groups`, using AgentGroup::add().
			 * 3. The location of each SpatialAgent is initialized to the
			 * `cell` they were built from, according to point 1. This is
			 * eventually performed using SpatialAgent::initLocation().
			 * 4. The DistributedMoveAlgorithm provided by
			 * `model.distributedMoveAlgorithm()` is executed to complete the
			 * location initialization of built \SpatialAgents.
			 *
			 *
			 * @param model model in which agent are initialized. More
			 * precisely, agents are initialized within the Cell network
			 * defined by `model.cells()`
			 * @param groups groups to which each agent must be added
			 * @param factory agent factory used to allocate agents
			 * @param agent_mapping agent mapping used to compute the number of
			 * agents to initialize on each cell
			 */
			virtual void build(
					SpatialModel<CellType>& model,
					GroupList groups,
					SpatialAgentFactory<CellType>& factory,
					SpatialAgentMapping<MappingCellType>& agent_mapping
					) = 0;

			/**
			 * Same as build(SpatialModel<CellType>&, GroupList, SpatialAgentFactory<CellType>&, SpatialAgentMapping<MappingCellType>&)
			 * but uses a call to `factory()` instead of the
			 * SpatialAgentFactory::build() method to build Agents.
			 *
			 * It might be convenient to initialize the `factory` from a lambda
			 * function:
			 * ```cpp
			 * int data;
			 *
			 * agent_builder.build(
			 * 	model, {agent_group},
			 * 	[&data] () {return new Agent(data);}, // Called only when required
			 * 	agent_mapping
			 * );
			 * ```
			 *
			 * @param model model in which agent are initialized. More
			 * precisely, agents are initialized within the Cell network
			 * defined by `model.cells()`
			 * @param groups groups to which each agent must be added
			 * @param factory callable object used to allocate agents
			 * @param agent_mapping agent mapping used to compute the number of
			 * agents to initialize on each cell
			 */
			virtual void build(
					SpatialModel<CellType>& model,
					GroupList groups,
					std::function<SpatialAgent<CellType>*()> factory,
					SpatialAgentMapping<MappingCellType>& agent_mapping
					) = 0;

			virtual ~SpatialAgentBuilder() {}
	};

}}}
#endif
