#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

/** \file src/fpmas/model/spatial/spatial_model.h
 * Spatial models implementation.
 */

#include "dist_move_algo.h"
#include "fpmas/api/model/exceptions.h"
#include "../serializer.h"

namespace fpmas {
	namespace model {
	using api::model::SpatialModelLayers;
	using api::model::DistributedId;

	/**
	 * FPMAS reserved api::model::GroupId used by Cell groups.
	 */
	static const api::model::GroupId CELL_GROUP_ID = -1;

	/**
	 * Can be used to specify that the current location of a SpatialAgent
	 * should be excluded from a Range.
	 */
	static const bool EXCLUDE_LOCATION = false;
	/**
	 * Can be used to specify that the current location of a SpatialAgent
	 * should be included in a Range.
	 */
	static const bool INCLUDE_LOCATION = true;

	/**
	 * api::model::MoveAgentGroup implementation.
	 */
	template<typename CellType>
	class MoveAgentGroup :
		public api::model::MoveAgentGroup<CellType>,
		public model::detail::AgentGroupBase {
		private:
			api::model::SpatialModel<CellType>& model;
			DistributedMoveAlgorithm<CellType> dist_move_algo;
		public:
			/**
			 * MoveAgentGroup constructor.
			 *
			 * The specified `behavior` is assumed to contain some
			 * SpatialAgent::moveTo() instructions. This is not required, but
			 * if it's not the case, the \DistributedMoveAlgorithm will be
			 * systematically executed while it's useless.
			 *
			 * @param group_id unique group id
			 * @param behavior behavior to execute on agents of the group
			 * @param model associated spatial model
			 * @param end_condition distributed move algorithm end condition
			 */
			MoveAgentGroup(
					api::model::GroupId group_id,
					const api::model::Behavior& behavior,
					api::model::SpatialModel<CellType>& model,
					api::model::EndCondition<CellType>& end_condition) :
			AgentGroupBase(group_id, model.graph(), behavior),
			model(model),
			dist_move_algo(model, *this, model.getGroup(CELL_GROUP_ID), end_condition) {
			}

			/**
			 * Returns a \JobList containing:
			 * 1. The execution of `behavior` on agents of the group, where
			 * SpatialAgent::moveTo() operations are potentially performed.
			 * 2. The execution of a \DistributedMoveAlgorithm, according to
			 * the specified `model`, to commit the previous `moveTo`
			 * operations and update agents location, mobility fields and
			 * perceptions. The algorithm is executed only on agents
			 * contained in the group. However, notice that those agents'
			 * perceptions are still updated with any other agents located on
			 * `model`'s cells, even if they are not included in this group.
			 *
			 * @return list of \Jobs associated to this \AgentGroup
			 */
			api::scheduler::JobList jobs() const override;

			api::model::DistributedMoveAlgorithm<CellType>& distributedMoveAlgorithm()  override {
				return dist_move_algo;
			}
	};
		
	template<typename CellType>
		api::scheduler::JobList MoveAgentGroup<CellType>::jobs() const {
			api::scheduler::JobList job_list;
			job_list.push_back(this->agentExecutionJob());

			for(auto job : dist_move_algo.jobs())
				job_list.push_back(job);
			return job_list;
		}


	/**
	 * api::model::Cell implementation.
	 *
	 * The api::model::Agent part of api::model::Cell is **not** implemented by
	 * this class.
	 *
	 * @see Cell
	 */
	class CellBase : public virtual api::model::Cell, public NeighborsAccess {
		private:
			std::set<DistributedId> move_flags;
			std::set<DistributedId> perception_flags;

			void updateLocation(
					api::model::Agent* agent, api::model::AgentEdge* new_location_edge
					);
			void growMobilityField(api::model::Agent* agent);
			void growPerceptionField(api::model::Agent* agent);

			/**
			 * Checks if `agent` is currently in `group`.
			 */
			bool isAgentInGroup(api::model::Agent* agent, api::model::AgentGroup& group);

			/*
			 * Some profiling analysis show that the successors() method calls,
			 * and more particularly the dynamic_casts that are involved, have
			 * a significant performance cost.
			 * The purpose of the following structures is to drastically reduce
			 * the call to the successors() method, buffering the result only
			 * when required.
			 * This is safe, since the Cell network can't be modified during
			 * the DistributedMoveAlgorithm execution.
			 * The buffer is safely used by init(), handleNewLocation(),
			 * handleMove() and handlePerceive(), but is not safe for a generic
			 * use since new CELL_SUCCESSOR edges might be deleted or created
			 * at any time, out of the DistributedMoveAlgorithm context.
			 */
			bool init_successors = false;
			std::vector<api::model::Cell*> successors_buffer;
			std::vector<api::model::AgentNode*> raw_successors_buffer;
			const std::vector<api::model::Cell*>& bufferedSuccessors();

		public:
			std::vector<api::model::Cell*> successors() override;

			void init() override;
			void handleNewLocation() override;
			void handleMove() override;
			void handlePerceive() override;

			void updatePerceptions(api::model::AgentGroup& group) override;
	};

	/**
	 * Complete api::model::Cell implementation.
	 *
	 * @tparam CellType final Cell type, that must inherit from the current
	 * Cell.
	 * @tparam TypeIdBase type used to define the type id of the current
	 * Cell implementation.
	 */
	template<typename CellType, typename TypeIdBase = CellType>
	class Cell : public CellBase, public AgentBase<CellType, TypeIdBase> {
	};

	/**
	 * api::model::SpatialModel implementation.
	 *
	 * @tparam SyncMode Synchronization Mode (see fpmas::synchro)
	 * @tparam CellType Cell type on which \SpatialAgents are moving. The
	 * specified type might be abstract.
	 * @tparam EndCondition DistributedMoveAlgorithm end condition. Uses
	 * DynamicEndCondition by default.
	 */
	template<
		template<typename> class SyncMode,
		typename CellType = api::model::Cell,
		typename EndCondition = DynamicEndCondition<CellType>>
	class SpatialModel :
		public api::model::SpatialModel<CellType>,
		public Model<SyncMode> {
		private:
			AgentGroup* cell_group;
			EndCondition dist_move_algo_end_condition;
			void buildCellGroup() {
				cell_group = &this->buildGroup(CELL_GROUP_ID);
			}

		public:
			/**
			 * Default SpatialModel constructor.
			 *
			 * The Model base is constructed using its default construtor.
			 */
			SpatialModel() {
				buildCellGroup();
			}

			/**
			 * SpatialModel constructor.
			 *
			 * The specified `load_balancing` algorithm is passed to the Model
			 * base construtor.
			 *
			 * @param load_balancing user defined load balancing algorithm
			 */
			SpatialModel(
					api::graph::LoadBalancing<api::model::AgentPtr>& load_balancing
					) : Model<SyncMode>(load_balancing) {
				buildCellGroup();
			}

			/**
			 * \copydoc fpmas::api::model::SpatialModel::add(CellType*)
			 */
			void add(CellType* cell) override;
			std::vector<CellType*> cells() override;
			fpmas::api::model::AgentGroup& cellGroup() override;
			MoveAgentGroup<CellType>& buildMoveGroup(
					api::model::GroupId id,
					const api::model::Behavior& behavior) override;
	};

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		void SpatialModel<SyncMode, CellType, EndCondition>::add(CellType* cell) {
			cell_group->add(cell);
		}

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		std::vector<CellType*> SpatialModel<SyncMode, CellType, EndCondition>::cells() {
			std::vector<CellType*> cells;
			for(auto agent : cell_group->localAgents())
				cells.push_back(dynamic_cast<CellType*>(agent));
			return cells;
		}

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		fpmas::api::model::AgentGroup& SpatialModel<SyncMode, CellType, EndCondition>::cellGroup() {
			return *cell_group;
		}

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		MoveAgentGroup<CellType>& SpatialModel<SyncMode, CellType, EndCondition>::buildMoveGroup(
				api::model::GroupId id, const api::model::Behavior& behavior) {
			auto* group = new MoveAgentGroup<CellType>(
					id, behavior, *this, dist_move_algo_end_condition);
			this->insert(id, group);
			return *group;
		}
	
	/**
	 * api::model::SpatialAgent API implementation.
	 *
	 * This is a partial implementation, that does not implement
	 * moveTo(CellType*).
	 *
	 * @tparam AgentType final SpatialAgentBase type (i.e. most derived class
	 * from this SpatialAgentBase)
	 * @tparam CellType type of cells on which the agent moves
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 *
	 * @see SpatialAgent
	 * @see GridAgent
	 */
	template<typename AgentType, typename CellType, typename Derived = AgentType>
	class SpatialAgentBase :
		public virtual api::model::SpatialAgent<CellType>,
		public model::AgentBase<AgentType, SpatialAgentBase<AgentType, CellType, Derived>> {
			friend nlohmann::adl_serializer<
				api::utils::PtrWrapper<SpatialAgentBase<AgentType, CellType, Derived>>>;
			friend fpmas::io::json::light_serializer<
				api::utils::PtrWrapper<SpatialAgentBase<AgentType, CellType, Derived>>>;


			public:
			/**
			 * The type that must be used in FPMAS_REGISTER_AGENT_TYPES and
			 * FPMAS_JSON_SET_UP to enable json serialization of `AgentType`,
			 * which is the dynamic type of any SpatialAgentBase<AgentType,
			 * CellType, Derived>.
			 *
			 * \par Example
			 * ```cpp
			 * class UserAgent : public fpmas::model::GridAgent<UserAgent> {
			 *    ...
			 * };
			 * 
			 * // alternatively, custom json serialization rules might be
			 * // defined
			 * FPMAS_DEFAULT_JSON(UserAgent)
			 *
			 * ...
			 *
			 * FPMAS_JSON_SET_UP(..., UserAgent::JsonBase, ...)
			 *
			 * int main(int argc, char** argv) {
			 *     FPMAS_REGISTER_AGENT_TYPES(..., UserAgent::JsonBase, ...)
			 *     ...
			 * }
			 * ```
			 */
			typedef SpatialAgentBase<AgentType, CellType, Derived> JsonBase;

			private:
				class CurrentOutLayer {
					private:
						SpatialAgentBase* _agent;
						fpmas::graph::LayerId layer_id;
						std::set<DistributedId> current_layer_ids;

					public:
						CurrentOutLayer(SpatialAgentBase* agent, fpmas::graph::LayerId layer_id)
							: _agent(agent), layer_id(layer_id) {
								auto layer = agent->node()->outNeighbors(layer_id);
								for(auto node : layer)
									current_layer_ids.insert(node->getId());
							}

						bool contains(fpmas::api::model::Agent* agent) {
							return current_layer_ids.count(agent->node()->getId()) > 0;
						}

						void link(fpmas::api::model::Agent* cell) {
							current_layer_ids.insert(cell->node()->getId());
							_agent->model()->link(_agent, cell, layer_id);
						}
				};
			private:
				DistributedId location_id;

				// Use pointers, since each instance (probably) belongs to the
				// implementing child class, or is static. In any case, it is
				// not the purpose of this base to perform copy assignment of
				// this fields, so a reference can't be used.
				const api::model::Range<CellType>* mobility_range;
				const api::model::Range<CellType>* perception_range;

			protected:
				/**
				 * SpatialAgentBase constructor.
				 *
				 * Typically, the specified member might initialized:
				 * - as static variables
				 * - as members of the derived SpatialAgentBase
				 *
				 * In any case, their storage durations must exceed the one
				 * this SpatialAgentBase.
				 *
				 * @param mobility_range mobility range
				 * @param perception_range perception range
				 */
				SpatialAgentBase(
						const api::model::Range<CellType>& mobility_range,
						const api::model::Range<CellType>& perception_range) :
					mobility_range(&mobility_range),
					perception_range(&perception_range) {}

				/**
				 * Updates this SpatialAgent location.
				 *
				 * 1. Links this agent to `cell` on the NEW_LOCATION layer.
				 * 2. Unlinks links on LOCATION, MOVE and PERCEPTION layers.
				 * 3. Updates the current locationId() with the `cell`s node
				 * id.
				 *
				 * This function is likely to be used by moveTo(CellType*)
				 * implementation, that is not implemented by this base.
				 *
				 * @param cell cell to which this agent should move
				 */
				void updateLocation(CellType* cell);

				/**
				 * \copydoc fpmas::api::model::SpatialAgent::handleNewMove
				 */
				void handleNewMove() override;
				/**
				 * \copydoc fpmas::api::model::SpatialAgent::handleNewPerceive
				 */
				void handleNewPerceive() override;

				using api::model::SpatialAgent<CellType>::moveTo;
				/**
				 * \copydoc fpmas::api::model::SpatialAgent::moveTo(DistributedId)
				 */
				void moveTo(DistributedId id) override;

				/**
				 * Returns \Cells currently contained in the agent's mobility
				 * field.
				 *
				 * @return mobility field
				 */
				Neighbors<CellType> mobilityField() const {
					return this->template outNeighbors<CellType>(SpatialModelLayers::MOVE);
				}

				/**
				 * Returns \Agents currently perceived by this agent.
				 *
				 * Only agents of type `NeighborType` are selected: other are
				 * ignored and not returned by this method.
				 *
				 * @tparam NeighborType type of perceived agents to return
				 * @return list of perceived \Agents of type `NeighborType`
				 */
				template<typename NeighborType = api::model::Agent>
					Neighbors<NeighborType> perceptions() const {
						return this->template outNeighbors<NeighborType>(SpatialModelLayers::PERCEPTION);
					}

			public:
				/**
				 * \copydoc fpmas::api::model::SpatialAgent::initLocation
				 */
				void initLocation(CellType* cell) override {
					this->moveTo(cell);
				}

				/**
				 * \copydoc fpmas::api::model::SpatialAgent::locationId
				 */
				DistributedId locationId() const override {
					return location_id;
				}

				/**
				 * \copydoc fpmas::api::model::SpatialAgent::locationCell
				 */
				CellType* locationCell() const override;

				/**
				 * \copydoc fpmas::api::model::SpatialAgent::mobilityRange
				 */
				const api::model::Range<CellType>& mobilityRange() const override {
					return *mobility_range;
				}

				/**
				 * \copydoc fpmas::api::model::SpatialAgent::perceptionRange
				 */
				const api::model::Range<CellType>& perceptionRange() const override {
					return *perception_range;
				}
		};

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::updateLocation(CellType* cell) {
			// Links to new location
			this->model()->link(this, cell, SpatialModelLayers::NEW_LOCATION);

			// Unlinks obsolete location
			auto location = this->template outNeighbors<api::model::Cell>(
					SpatialModelLayers::LOCATION);
			if(location.count() > 0)
				this->model()->unlink(location.at(0).edge());

			// Unlinks obsolete mobility field
			for(auto _cell : this->template outNeighbors<api::model::Cell>(
						SpatialModelLayers::MOVE)) {
				this->model()->unlink(_cell.edge());
			}

			// Unlinks obsolete perception field
			for(auto agent : this->template outNeighbors<api::model::Cell>(
						SpatialModelLayers::PERCEIVE))
				this->model()->unlink(agent.edge());


			// Adds the NEW_LOCATION to the mobility/perceptions fields
			// depending on the current ranges
			if(this->mobility_range->contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::MOVE);
			if(this->perception_range->contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::PERCEIVE);

			this->location_id = cell->node()->getId();
		}

	template<typename AgentType, typename CellType, typename Derived>
		CellType* SpatialAgentBase<AgentType, CellType, Derived>::locationCell() const {
			auto location = this->template outNeighbors<CellType>(
					SpatialModelLayers::LOCATION);
			if(location.count() > 0)
				return location.at(0);
			return nullptr;
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewMove() {

			CurrentOutLayer move_layer(this, SpatialModelLayers::MOVE);

			auto location =  this->locationCell();
			fpmas::model::ReadGuard read_location(location);
			for(auto cell_edge : this->node()->getOutgoingEdges(SpatialModelLayers::NEW_MOVE)) {
				auto* agent = cell_edge->getTargetNode()->data().get();
				fpmas::model::ReadGuard read_cell(agent);
				if(!move_layer.contains(agent)
						&& this->mobility_range->contains(location, dynamic_cast<CellType*>(agent)))
					move_layer.link(agent);
				this->model()->unlink(cell_edge);
			}
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewPerceive() {
			CurrentOutLayer perceive_layer(this, SpatialModelLayers::PERCEIVE);

			auto location =  this->locationCell();
			fpmas::model::ReadGuard read_location(location);
			for(auto cell_edge : this->node()->getOutgoingEdges(SpatialModelLayers::NEW_PERCEIVE)) {
				auto* agent = cell_edge->getTargetNode()->data().get();
				fpmas::model::ReadGuard read_cell(agent);
				if(!perceive_layer.contains(agent)
						&& this->perception_range->contains(location, dynamic_cast<CellType*>(agent)))
					perceive_layer.link(agent);
				this->model()->unlink(cell_edge);
			}
		}
	
	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::moveTo(DistributedId cell_id) {
			bool found = false;
			auto mobility_field = this->template outNeighbors<CellType>(SpatialModelLayers::MOVE);
			auto it = mobility_field.begin();
			while(!found && it != mobility_field.end()) {
				if((*it)->node()->getId() == cell_id) {
					found=true;
				} else {
					it++;
				}
			}
			if(found)
				this->moveTo(*it);
			else
				throw api::model::OutOfMobilityFieldException(this->node()->getId(), cell_id);
		}

	/**
	 * SpatialAgent that can be used in an arbitrary graph environment.
	 *
	 * @tparam AgentType final SpatialAgent type (i.e. most derived class from
	 * this SpatialAgent)
	 * @tparam CellType type of cells on which the agent moves
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 */
	template<typename AgentType, typename CellType = api::model::Cell, typename Derived = AgentType>
		class SpatialAgent :
			public SpatialAgentBase<AgentType, CellType, Derived> {
				protected:
					using SpatialAgentBase<AgentType, CellType, Derived>::moveTo;
					using SpatialAgentBase<AgentType, CellType, Derived>::SpatialAgentBase;

					/**
					 * \copydoc fpmas::api::model::SpatialAgent::moveTo(CellType*)
					 */
					void moveTo(CellType* cell) override {
						this->updateLocation(cell);
					}
			};

	/**
	 * api::model::SpatialAgentFactory implementation that uses `AgentType`s
	 * default constructor.
	 */
	template<typename AgentType>
		class DefaultSpatialAgentFactory : public api::model::SpatialAgentFactory<typename AgentType::Cell> {
			public:
				/**
				 * Returns `new AgentType()`.
				 *
				 * @return dynamically allocated, default constructed
				 * SpatialAgent
				 */
				AgentType* build() override {
					return new AgentType;
				}
		};

	/**
	 * An api::model::SpatialAgentMapping implementation that randomly assign
	 * \SpatialAgents to available \Cells.
	 */
	class UniformAgentMapping : public api::model::SpatialAgentMapping<api::model::Cell> {
		private:
			std::unordered_map<DistributedId, std::size_t> mapping;

		public:
			/**
			 * UniformAgentMapping constructor.
			 *
			 * `agent_count` agents are distributed on the distributed set of
			 * \Cells contained in the specified `cell_group`.
			 *
			 * A custom `seed` might be specified. However, since the generated
			 * mapping is required to be the same on **all processes**, the
			 * same `seed` must be specified on all processes.
			 *
			 * For the same reasons, the same `agent_count` must be specified
			 * on all processes, since this corresponds to the **global** count
			 * of \SpatialAgents to initialize.
			 *
			 * @param comm MPI communicator
			 * @param cell_group AgentGroup containing \Cells to which a
			 * \SpatialAgent count will be assigned.
			 * @param agent_count total number of \SpatialAgents to distribute
			 * on a set of \Cells. 
			 * @param seed random seed
			 */
			UniformAgentMapping(
					api::communication::MpiCommunicator& comm,
					api::model::AgentGroup& cell_group,
					std::size_t agent_count,
					std::mt19937_64::result_type seed = std::mt19937_64::default_seed
					);

			std::size_t countAt(api::model::Cell* cell) override;
	};

	/**
	 * api::model::SpatialAgentBuilder implementation.
	 */
	template<typename CellType, typename MappingCellType = api::model::Cell>
	class SpatialAgentBuilder : public api::model::SpatialAgentBuilder<CellType, MappingCellType>{
		public:
			/**
			 * FPMAS reserved group id used by the SpatialAgentBuilder
			 * temporary group used to build() agents.
			 */
			static const api::model::GroupId TEMP_GROUP_ID = -2;

			/**
			 * \copydoc api::model::SpatialAgentBuilder::build
			 */
			void build(
					api::model::SpatialModel<CellType>& model,
					api::model::GroupList groups,
					api::model::SpatialAgentFactory<CellType>& factory,
					api::model::SpatialAgentMapping<MappingCellType>& agent_mapping
					) override;
	};

	template<typename CellType, typename MappingCellType>
		void SpatialAgentBuilder<CellType, MappingCellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups,
				api::model::SpatialAgentFactory<CellType>& factory,
				api::model::SpatialAgentMapping<MappingCellType>& agent_mapping
				) {
			model::DefaultBehavior _;
			api::model::MoveAgentGroup<CellType>& temp_group = model.buildMoveGroup(TEMP_GROUP_ID, _);
			std::vector<api::model::SpatialAgent<CellType>*> agents;
			for(auto cell : model.cells()) {
				for(std::size_t i = 0; i < agent_mapping.countAt(cell); i++) {
					auto agent = factory.build();
					agents.push_back(agent);
					temp_group.add(agent);
					for(api::model::AgentGroup& group : groups)
						group.add(agent);
					agent->initLocation(cell);
				}
			}

			model.runtime().execute(
					temp_group.distributedMoveAlgorithm().jobs()
					);
			model.removeGroup(temp_group);
		}
}}

namespace nlohmann {
	/**
	 * Polymorphic SpatialAgentBase nlohmann json serializer specialization.
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			/**
			 * Serializes the pointer to the polymorphic SpatialAgentBase using
			 * the following JSON schema:
			 * ```json
			 * [<Derived json serialization>, ptr->locationId()]
			 * ```
			 *
			 * The `<Derived json serialization>` is computed using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>>`
			 * specialization, that can be defined externally without
			 * additional constraint.
			 *
			 * @param j json output
			 * @param ptr pointer to a polymorphic SpatialAgentBase to serialize
			 */
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				j[1] = ptr->location_id;
			}

			/**
			 * Unserializes a polymorphic SpatialAgentBase 
			 * from the specified Json.
			 *
			 * First, the `Derived` part, that extends `SpatialAgentBase` by
			 * definition, is unserialized from `j[0]` using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization, that can be defined externally without any
			 * constraint. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `SpatialAgentBase`
			 * members undefined. The specific `SpatialAgentBase` member
			 * `location_id` is then initialized from `j[1]`, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `SpatialAgentBase` pointer.
			 *
			 * @param j json input
			 * @return unserialized pointer to a polymorphic `SpatialAgentBase`
			 */
			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				derived_ptr->location_id = j[1].get<fpmas::api::graph::DistributedId>();
				return derived_ptr.get();
			}
		};
}

namespace fpmas { namespace io { namespace json {
	/**
	 * light_serializer specialization for an fpmas::model::SpatialAgentBase
	 *
	 * The light_serializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current \light_json.
	 *
	 * @tparam AgentType final \Agent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct light_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			
			/**
			 * \light_json to_json() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_json()`,
			 * without adding any `SpatialAgentBase` specific data to the
			 * \light_json j.
			 *
			 * @param j json output
			 * @param agent agent to serialize 
			 */
			static void to_json(light_json& j, const Ptr& agent) {
				// Derived serialization
				light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::to_json(
						j,
						const_cast<Derived*>(dynamic_cast<const Derived*>(agent.get()))
						);
			}

			/**
			 * \light_json from_json() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_json()`,
			 * without extracting any `SpatialAgentBase` specific data from the
			 * \light_json j.
			 *
			 * @param j json input
			 * @return dynamically allocated `Derived` instance, unserialized from `j`
			 */
			static Ptr from_json(const light_json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::from_json(j);
				return derived_ptr.get();
			}
		};

}}}
#endif
