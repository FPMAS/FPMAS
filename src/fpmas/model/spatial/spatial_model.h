#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

/** \file src/fpmas/model/spatial/spatial_model.h
 * Spatial models implementation.
 */

#include "dist_move_algo.h"
#include "fpmas/api/model/exceptions.h"
#include "../serializer.h"


/**
 * Utility macro to define the fpmas::api::model::SpatialAgent::mobilityRange()
 * method for the current agent so that it returns the specified RANGE.
 *
 * This macro must be called within the class definition of a SpatialAgent.
 *
 * Using this macro is not required: the mobilityRange() method can be manually
 * defined.
 *
 * @param RANGE reference to the agent mobility range. The specified range
 * might be a class member, a static variable, or any other variable which
 * lifetime exceeds the one of the current agent.
 */
#define FPMAS_MOBILITY_RANGE(RANGE)\
	const decltype(RANGE)& mobilityRange() const override {return RANGE;}

/**
 * Utility macro to define the
 * fpmas::api::model::SpatialAgent::perceptionRange() method for the current
 * agent so that it returns the specified RANGE.
 *
 * This macro must be called within the class definition of a SpatialAgent.
 *
 * Using this macro is not required: the perceptionRange() method can be
 * manually defined.
 *
 * @param RANGE reference to the agent perception range. The specified range
 * might be a class member, a static variable, or any other variable which
 * lifetime exceeds the one of the current agent.
 */
#define FPMAS_PERCEPTION_RANGE(RANGE)\
	const decltype(RANGE)& perceptionRange() const override {return RANGE;}

namespace fpmas { namespace model {
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
					 * SpatialAgent::moveTo() operations are potentially performed,
					 * including any necessary synchronization at the end of all
					 * behavior executions. Corresponds to the agentExecutionJob() in
					 * practice.
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

	namespace detail {
		/**
		 * Utility class used to describe agents currently contained in the current
		 * outgoing neighbors of an agent, on a given layer.
		 */
		class CurrentOutLayer {
			private:
				api::model::Agent* _agent;
				fpmas::graph::LayerId layer_id;
				std::vector<DistributedId> current_layer_ids;

			public:
				/**
				 * CurrentOutLayer constructor.
				 *
				 * @param agent root agent
				 * @param layer_id id of the layer on which outgoing neighbors
				 * of the agent are considered
				 */
				CurrentOutLayer(api::model::Agent* agent, fpmas::graph::LayerId layer_id)
					: _agent(agent), layer_id(layer_id) {
						auto edges = agent->node()->getOutgoingEdges(layer_id);
						current_layer_ids.resize(edges.size());

						for(std::size_t i = 0; i < edges.size(); i++)
							current_layer_ids[i] = edges[i]->getTargetNode()->getId();
					}

				/**
				 * Checks if the input agent is contained in the current
				 * outgoing neighbors of the root agent on the current layer.
				 *
				 * @param agent agent to check
				 * @return true iff the `agent` is contained in the outgoing
				 * neighbors on the layer
				 */
				bool contains(fpmas::api::model::Agent* agent) {
					return std::find(
							current_layer_ids.begin(),
							current_layer_ids.end(),
							agent->node()->getId()
							) != current_layer_ids.end();
				}

				/**
				 * Builds a link from the root agent to the provided other
				 * agent on the current layer.
				 * This method allows to automatically update the
				 * CurrentOutLayer, while directly calling the `link` operation
				 * does not.
				 *
				 * @param other_agent agent to link
				 */
				void link(fpmas::api::model::Agent* other_agent) {
					current_layer_ids.push_back(other_agent->node()->getId());
					_agent->model()->link(_agent, other_agent, layer_id);
				}
		};
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
			void updateLocation(
					api::model::Agent* agent, api::model::AgentEdge* new_location_edge
					);


		protected:
			/**
			 * A local set of ids of agents that have **not** moved since the
			 * last DistributedMoveAlgorithm execution.
			 *
			 * **internal use only**
			 */
			std::set<DistributedId> no_move_flags;
			/**
			 * A local set of ids of agents that have explored the current cell
			 * on the MOVE layer during the current DistributedMoveAlgorithm
			 * execution.
			 *
			 * **internal use only**
			 */
			std::set<DistributedId> move_flags;
			/**
			 * A local set of ids of agents that have explored the current cell
			 * on the PERCEIVE layer during the current DistributedMoveAlgorithm
			 * execution.
			 *
			 * **internal use only**
			 */
			std::set<DistributedId> perception_flags;

			// The two following methods are virtual since their implementation
			// require the CellType template parameter, not defined in this
			// class. They are implemented in the Cell class below.

			/**
			 * Grows the current `agent`s mobility field, connecting it to
			 * successors() on the NEW_MOVE layer if the agent is DISTANT, or
			 * directly on the MOVE layer otherwise if the cell is not already in
			 * the move layer and is contained in the agent mobility range.
			 */
			virtual void growMobilityField(api::model::Agent* agent) = 0;
			/**
			 * Grows the current `agent`s perception field, connecting it to
			 * successors() on the NEW_PERCEIVE layer if the agent is DISTANT, or
			 * directly on the PERCEIVE layer otherwise if the cell is not already in
			 * the PERCEIVE layer and is contained in the agent perception range.
			 */
			virtual void growPerceptionField(api::model::Agent* agent) = 0;

		public:
			std::vector<api::model::Cell*> successors() override;

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
			private:
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
				std::vector<CellType*> successors_buffer;
				std::vector<api::model::AgentEdge*> raw_successors_buffer;
				const std::vector<CellType*>& bufferedSuccessors();


			public:
				/**
				 * \copydoc fpmas::api::model::CellBehavior::init()
				 */
				void init() override;

			private:
				void growMobilityField(api::model::Agent* agent) override;
				void growPerceptionField(api::model::Agent* agent) override;
		};

	template<typename CellType, typename TypeIdBase>
		const std::vector<CellType*>& Cell<CellType, TypeIdBase>::bufferedSuccessors() {
			return this->successors_buffer;
		}

	template<typename CellType, typename TypeIdBase>
		void Cell<CellType, TypeIdBase>::init() {
			bool init_successors;

			// This has no performance impact
			auto current_successors
				= this->node()->getOutgoingEdges(SpatialModelLayers::CELL_SUCCESSOR);
			// Checks if the currently buffered successors are stricly equal to the
			// current_successors. In this case, there is no need to update the
			// successors() list
			if(current_successors.size() == 0 ||
					current_successors.size() != raw_successors_buffer.size()) {
				init_successors = false;
			} else {
				auto it = current_successors.begin();
				auto raw_it = raw_successors_buffer.begin();
				while(it != current_successors.end() && (*it) == *raw_it) {
					it++;
					raw_it++;
				}
				if(it == current_successors.end()) {
					init_successors = true;
				} else {
					init_successors = false;
				}
			}

			if(!init_successors) {
				raw_successors_buffer = current_successors;
				successors_buffer.resize(raw_successors_buffer.size());
				for(std::size_t i = 0; i < raw_successors_buffer.size(); i++)
					successors_buffer[i] = dynamic_cast<CellType*>(
							raw_successors_buffer[i]->getTargetNode()->data().get()
							);
			}

			std::set<DistributedId> new_location_layer;
			for(auto agent_edge
					: this->node()->getIncomingEdges(SpatialModelLayers::NEW_LOCATION))
				new_location_layer.insert(agent_edge->getSourceNode()->getId());

			// The location of agents that are still linked on the MOVE /
			// PERCEIVE layer has not been updated, since those layers are
			// unlinked by SpatialAgent::updateLocation(), unless the current
			// cell is actually their NEW_LOCATION.
			for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::MOVE)) {
				if(new_location_layer.count(agent_edge->getSourceNode()->getId())==0)
					this->no_move_flags.insert(agent_edge->getSourceNode()->getId());
			}
			for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::PERCEIVE)) {
				if(new_location_layer.count(agent_edge->getSourceNode()->getId())==0)
					this->no_move_flags.insert(agent_edge->getSourceNode()->getId());
			}
			// All agents already linked on the LOCATION layer didn't update
			// their location, since LOCATION links are unlinked by
			// SpatialAgent::updateLocation().
			for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::LOCATION)) {
				this->no_move_flags.insert(agent_edge->getSourceNode()->getId());
			}
		}

	template<typename CellType, typename TypeIdBase>
		void Cell<CellType, TypeIdBase>::growMobilityField(api::model::Agent* agent) {
			if(agent->node()->state() == api::graph::LOCAL) {
				// There is no need to create a temporary NEW_MOVE
				// link, since the mobilityRange() of the agent is
				// directly available on the current process.
				auto* spatial_agent
					= dynamic_cast<api::model::SpatialAgent<CellType>*>(agent);

				// Valid, since the agent is LOCAL
				auto location = spatial_agent->locationCell();

				fpmas::model::ReadGuard read_location(location);

				// Cells currently in the move_layer
				detail::CurrentOutLayer move_layer(agent, SpatialModelLayers::MOVE);
				for(auto cell : this->bufferedSuccessors()) {
					if(!move_layer.contains(cell)) {
						// The cell as not already been linked in the
						// MOVE layer
						fpmas::model::ReadGuard read_cell(cell);
						if(spatial_agent->mobilityRange().contains(
									location,
									cell
									)) {
							// The cell is contained in the agent
							// mobility range: add it to its mobility
							// field
							move_layer.link(cell);
						}
					}
				}
			} else {
				// In this case, the mobility range of the agent is not
				// available on the current process, so a temporary
				// NEW_MOVE edge must be created and handled later by
				// the agent in the SpatialAgent::handleNewMove()
				// method
				for(auto cell : this->bufferedSuccessors())
					this->model()->link(
							agent,
							cell,
							SpatialModelLayers::NEW_MOVE
							);
			}
		}

	template<typename CellType, typename TypeIdBase>
		void Cell<CellType, TypeIdBase>::growPerceptionField(api::model::Agent* agent) {
			// Exactly the same process as growMobilityField().
			if(agent->node()->state() == api::graph::LOCAL) {
				auto* spatial_agent
					= dynamic_cast<api::model::SpatialAgent<CellType>*>(agent);

				auto location =  spatial_agent->locationCell();
				fpmas::model::ReadGuard read_location(location);

				detail::CurrentOutLayer perceive_layer(agent, SpatialModelLayers::PERCEIVE);
				for(auto cell : this->bufferedSuccessors()) {
					if(!perceive_layer.contains(cell)) {
						fpmas::model::ReadGuard read_cell(cell);
						if(spatial_agent->perceptionRange().contains(
									location,
									cell
									)) {
							perceive_layer.link(cell);
						}
					}
				}
			} else {
				for(auto cell : this->bufferedSuccessors())
					this->model()->link(
							agent,
							cell,
							SpatialModelLayers::NEW_PERCEIVE
							);
			}
		}
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
				public virtual api::model::SpatialModel<CellType>,
				public Model<SyncMode> {
					private:
						AgentGroup& cell_group {this->buildGroup(CELL_GROUP_ID)};
						EndCondition dist_move_algo_end_condition;

					public:
						using Model<SyncMode>::Model;

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
			cell_group.add(cell);
		}

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		std::vector<CellType*> SpatialModel<SyncMode, CellType, EndCondition>::cells() {
			std::vector<CellType*> cells;
			for(auto agent : cell_group.localAgents())
				cells.push_back(dynamic_cast<CellType*>(agent));
			return cells;
		}

	template<template<typename> class SyncMode, typename CellType, typename EndCondition>
		fpmas::api::model::AgentGroup& SpatialModel<SyncMode, CellType, EndCondition>::cellGroup() {
			return cell_group;
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
				friend fpmas::io::datapack::Serializer<
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
				mutable CellType* location_cell_buffer = nullptr;
				DistributedId location_id;

				protected:
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
			};

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::updateLocation(CellType* cell) {
			// Links to new location
			this->model()->link(this, cell, SpatialModelLayers::NEW_LOCATION);

			// Unlinks obsolete location
			auto location = this->node()->getOutgoingEdges(SpatialModelLayers::LOCATION);
			if(location.size() > 0)
				this->model()->unlink(location[0]);

			// Unlinks obsolete mobility field
			for(auto cell_edge :
					this->node()->getOutgoingEdges(SpatialModelLayers::MOVE)) {
				this->model()->unlink(cell_edge);
			}

			// Unlinks obsolete perception field
			for(auto cell_edge
					: this->node()->getOutgoingEdges(SpatialModelLayers::PERCEIVE))
				this->model()->unlink(cell_edge);

			// Unlinks obsolete perceptions
			for(auto perception
					: this->node()->getOutgoingEdges(fpmas::api::model::PERCEPTION))
				this->model()->unlink(perception);


			// Adds the NEW_LOCATION to the mobility/perceptions fields
			// depending on the current ranges
			if(this->mobilityRange().contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::MOVE);
			if(this->perceptionRange().contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::PERCEIVE);

			this->location_id = cell->node()->getId();
			this->location_cell_buffer = cell;
		}

	template<typename AgentType, typename CellType, typename Derived>
		CellType* SpatialAgentBase<AgentType, CellType, Derived>::locationCell() const {
			if(location_cell_buffer != nullptr) {
				return location_cell_buffer;
			} else {
				auto edges = this->node()->getOutgoingEdges(SpatialModelLayers::LOCATION);
				if(!edges.empty())
					location_cell_buffer = dynamic_cast<CellType*>(
							edges[0]->getTargetNode()->data().get()
							);
				else
					location_cell_buffer = nullptr;
			}
			/*
			 *auto location = this->template outNeighbors<CellType>(
			 *        SpatialModelLayers::LOCATION);
			 *if(location.count() > 0)
			 *    return location.at(0);
			 *return nullptr;
			 */
			return location_cell_buffer;
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewMove() {

			detail::CurrentOutLayer move_layer(this, SpatialModelLayers::MOVE);

			auto location =  this->locationCell();
			fpmas::model::ReadGuard read_location(location);
			for(auto cell_edge : this->node()->getOutgoingEdges(SpatialModelLayers::NEW_MOVE)) {
				auto* agent = cell_edge->getTargetNode()->data().get();
				fpmas::model::ReadGuard read_cell(agent);
				if(!move_layer.contains(agent)
						&& this->mobilityRange().contains(location, dynamic_cast<CellType*>(agent)))
					move_layer.link(agent);
				this->model()->unlink(cell_edge);
			}
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewPerceive() {
			detail::CurrentOutLayer perceive_layer(this, SpatialModelLayers::PERCEIVE);

			auto location =  this->locationCell();
			fpmas::model::ReadGuard read_location(location);
			for(auto cell_edge : this->node()->getOutgoingEdges(SpatialModelLayers::NEW_PERCEIVE)) {
				auto* agent = cell_edge->getTargetNode()->data().get();
				fpmas::model::ReadGuard read_cell(agent);
				if(!perceive_layer.contains(agent)
						&& this->perceptionRange().contains(location, dynamic_cast<CellType*>(agent)))
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
	 * A range that does not contain any cell, not even the location cell.
	 *
	 * @tparam CellType type of cells on which the agent moves
	 */
	template<typename CellType>
		struct VoidRange : public api::model::Range<CellType> {
			/**
			 * Always returns false: the range does not contain anything.
			 */
			bool contains(CellType*, CellType*) const override {
				return false;
			}

			/**
			 * Returns 0.
			 */
			std::size_t radius(CellType*) const override {
				return 0;
			}
		};

	/**
	 * A range that only contains the current location of an agent, i.e. the
	 * origin of the range.
	 *
	 * @tparam CellType type of cells on which the agent moves
	 */
	template<typename CellType>
		struct LocationRange : public api::model::Range<CellType> {
			/**
			 * Returns true iff `cell` is `location`.
			 */
			bool contains(CellType* location, CellType* cell) const override {
				return location->node()->getId() == cell->node()->getId();
			}

			/**
			 * Returns 0.
			 */
			std::size_t radius(CellType*) const override {
				return 0;
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
				 * \copydoc api::model::SpatialAgentBuilder::build(SpatialModel<CellType>&, GroupList, SpatialAgentFactory<CellType>&, SpatialAgentMapping<MappingCellType>&)
				 */
				void build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups,
						api::model::SpatialAgentFactory<CellType>& factory,
						api::model::SpatialAgentMapping<MappingCellType>& agent_mapping
						) override;

				/**
				 * \copydoc api::model::SpatialAgentBuilder::build(SpatialModel<CellType>&, GroupList, std::function<SpatialAgent<CellType>*()>, SpatialAgentMapping<MappingCellType>&)
				 */
				void build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups,
						std::function<api::model::SpatialAgent<CellType>*()> factory,
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
			build(model, groups, [&factory] () {return factory.build();}, agent_mapping);
		}

	template<typename CellType, typename MappingCellType>
		void SpatialAgentBuilder<CellType, MappingCellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups,
				std::function<api::model::SpatialAgent<CellType>*()> factory,
				api::model::SpatialAgentMapping<MappingCellType>& agent_mapping
				) {
			model::DefaultBehavior _;
			api::model::MoveAgentGroup<CellType>& temp_group
				= model.buildMoveGroup(TEMP_GROUP_ID, _);
			std::vector<api::model::SpatialAgent<CellType>*> agents;
			for(auto cell : model.cells()) {
				for(std::size_t i = 0; i < agent_mapping.countAt(cell); i++) {
					auto agent = factory();
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
		struct light_serializer<PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			
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
				light_serializer<PtrWrapper<Derived>>::to_json(
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
				PtrWrapper<Derived> derived_ptr
					= light_serializer<PtrWrapper<Derived>>::from_json(j);
				return derived_ptr.get();
			}
		};

}}}

namespace fpmas { namespace io { namespace datapack {
	/**
	 * Polymorphic SpatialAgentBase ObjectPack serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` ObjectPack serialization | SpatialAgentBase::locationId() |
	 *
	 * The `Derived` part is serialized using the
	 * Serializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint. The input GridAgent pointer is
	 * dynamically cast to `Derived` when required to call the proper
	 * Serializer specialization.
	 *
	 * @tparam AgentType final fpmas::api::model::SpatialAgent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct Serializer<PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * SpatialAgentBase pointed by `ptr` to `p`.
			 */
			static std::size_t size(const ObjectPack& p, const Ptr& ptr) {
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				return p.template size(derived) + p.template size<DistributedId>();
			}

			/**
			 * Serializes the pointer to the polymorphic SpatialAgentBase into the
			 * specified ObjectPack.
			 *
			 * @param pack destination ObjectPack
			 * @param ptr pointer to a polymorphic SpatialAgentBase to serialize
			 */
			static void to_datapack(ObjectPack& pack, const Ptr& ptr) {
				// Derived serialization
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				pack.put(derived);
				pack.put(ptr->locationId());
			}

			/**
			 * Unserializes a polymorphic SpatialAgentBase 
			 * from the specified ObjectPack.
			 *
			 * First, the `Derived` part, that extends `SpatialAgentBase` by
			 * definition, is unserialized using the
			 * `Serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `SpatialAgentBase`
			 * members undefined. The specific `SpatialAgentBase` member
			 * `location_id` is then unserialized, and the unserialized
			 * `Derived` instance is returned in the form of a polymorphic
			 * `SpatialAgentBase` pointer.
			 *
			 * @param pack source ObjectPack
			 * @return unserialized pointer to a polymorphic `SpatialAgentBase`
			 */
			static Ptr from_datapack(const ObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr = pack
					.get<PtrWrapper<Derived>>();

				derived_ptr->location_id = pack.get<fpmas::api::graph::DistributedId>();
				return derived_ptr.get();
			}
		};

	/**
	 * LightSerializer specialization for an fpmas::model::SpatialAgentBase
	 *
	 * The LightSerializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current LightObjectPack.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` LightObjectPack serialization |
	 *
	 * The `Derived` part is serialized using the
	 * LightSerializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint (and potentially leaves the
	 * LightObjectPack empty).
	 *
	 * @tparam AgentType final \Agent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct LightSerializer<PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			
			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * \Agent pointed to by `ptr` to `p`.
			 */
			static std::size_t size(const LightObjectPack& p, const Ptr& ptr) {
				return p.size(PtrWrapper<Derived>(
							const_cast<Derived*>(
								dynamic_cast<const Derived*>(ptr.get())
								)
							));
			}

			/**
			 * LightObjectPack to_datapack() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_datapack()`,
			 * without adding any `SpatialAgentBase` specific data to the
			 * LightObjectPack o.
			 *
			 * @param o object pack output
			 * @param agent agent to serialize 
			 */
			static void to_datapack(LightObjectPack& o, const Ptr& agent) {
				// Derived serialization
				LightSerializer<PtrWrapper<Derived>>::to_datapack(
						o,
						const_cast<Derived*>(dynamic_cast<const Derived*>(agent.get()))
						);
			}

			/**
			 * LightObjectPack from_datapack() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `LightObjectPack<fpmas::api::utils::PtrWrapper<Derived>>::%from_datapack()`,
			 * without extracting any `SpatialAgentBase` specific data from the
			 * LightObjectPack j.
			 *
			 * @param o object pack input
			 * @return dynamically allocated `Derived` instance, unserialized from `j`
			 */
			static Ptr from_datapack(const LightObjectPack& o) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr
					= LightSerializer<PtrWrapper<Derived>>::from_datapack(o);
				return derived_ptr.get();
			}
		};
}}}
#endif
