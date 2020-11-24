#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "fpmas/api/model/spatial_model.h"
#include "fpmas/api/model/exceptions.h"
#include "model.h"
#include "serializer.h"

namespace fpmas {
	namespace model {
	using api::model::SpatialModelLayers;

	class MoveAgentGroup : public model::detail::AgentGroupBase {
		private:
			api::model::SpatialModel& model;
		public:
			MoveAgentGroup(
					api::model::GroupId group_id,
					api::model::AgentGraph& agent_graph,
					api::model::SpatialModel& model);

			MoveAgentGroup(
					api::model::GroupId group_id,
					api::model::AgentGraph& agent_graph,
					const api::model::Behavior& behavior,
					api::model::SpatialModel& model);

			api::scheduler::JobList jobs() const;
	};

	class CellBase : public virtual api::model::Cell, public NeighborsAccess {
		private:
			std::set<fpmas::api::graph::DistributedId> move_flags;
			std::set<fpmas::api::graph::DistributedId> perception_flags;

		protected:
			void updateLocation(api::model::Neighbor<api::model::Agent>& agent);
			void growMobilityRange(api::model::Agent* agent);
			void growPerceptionRange(api::model::Agent* agent);

		public:
			std::vector<api::model::Cell*> neighborhood() override;

			void handleNewLocation() override;
			void handleMove() override;
			void handlePerceive() override;

			void updatePerceptions() override;
	};

	template<typename CellType, typename TypeIdBase = CellType>
	class Cell : public CellBase, public AgentBase<CellType, TypeIdBase> {
	};

	enum CellGroups : api::model::GroupId {
		CELL_UPDATE_RANGES = -1,
		CELL_UPDATE_PERCEPTIONS = -2,
		AGENT_CROP_RANGES = -3,
	};

	class SpatialModelBase : public api::model::SpatialModel {
		private:
			AgentGroup* cell_behavior_group;
			Behavior<api::model::CellBehavior> cell_behaviors {
				&api::model::CellBehavior::handleNewLocation,
				&api::model::CellBehavior::handleMove,
				&api::model::CellBehavior::handlePerceive
			};
			AgentGroup* spatial_agent_group;
			Behavior<api::model::SpatialAgentBehavior> spatial_agent_behaviors {
				&api::model::SpatialAgentBehavior::handleNewMove,
				&api::model::SpatialAgentBehavior::handleNewPerceive
			};
			AgentGroup* update_perceptions_group;
			Behavior<api::model::CellBehavior> update_perceptions_behavior {
				&api::model::CellBehavior::updatePerceptions
			};
			unsigned int max_mobility_range;
			unsigned int max_perception_range;

		protected:
			void initGroups() {
				cell_behavior_group = &this->buildGroup(
							CELL_UPDATE_RANGES, cell_behaviors);
				spatial_agent_group = &this->buildGroup(
							AGENT_CROP_RANGES, spatial_agent_behaviors);
				update_perceptions_group = &this->buildGroup(
							CELL_UPDATE_PERCEPTIONS, update_perceptions_behavior);
			}

			SpatialModelBase(
					unsigned int max_mobility_range,
					unsigned int max_perception_range);
		public:
			void add(api::model::SpatialAgentBase* agent) override;
			void add(api::model::Cell* cell) override;
			std::vector<api::model::Cell*> cells() override;
			api::model::AgentGroup& buildMoveGroup(
					api::model::GroupId id, const api::model::Behavior& behavior) override;

			api::scheduler::JobList distributedMoveAlgorithm() override;
	};

	template<template<typename> class SyncMode>
	class SpatialModel : public SpatialModelBase, public Model<SyncMode> {
		public:
			SpatialModel(
					unsigned int max_mobility_range,
					unsigned int max_perception_range)
				: SpatialModelBase(max_mobility_range, max_perception_range) {
					this->initGroups();
				}
	};

	
	template<typename AgentType, typename CellType, typename Derived = AgentType>
	class SpatialAgentBase :
		public virtual api::model::SpatialAgent,
		public model::AgentBase<AgentType, SpatialAgentBase<AgentType, CellType, Derived>> {
			friend nlohmann::adl_serializer<
				api::utils::PtrWrapper<SpatialAgentBase<AgentType, CellType, Derived>>>;

			public:
				typedef SpatialAgentBase<AgentType, CellType, Derived> JsonBase;

			private:
				class CurrentOutLayer {
					private:
						SpatialAgentBase* agent;
						fpmas::graph::LayerId layer_id;
						std::set<DistributedId> current_layer_ids;

					public:
						CurrentOutLayer(SpatialAgentBase* agent, fpmas::graph::LayerId layer_id)
							: agent(agent), layer_id(layer_id) {
								auto layer = agent->node()->outNeighbors(layer_id);
								for(auto node : layer)
									current_layer_ids.insert(node->getId());
							}

						bool contains(fpmas::api::model::Agent* agent) {
							return current_layer_ids.count(agent->node()->getId()) > 0;
						}

						void link(fpmas::api::model::Agent* cell) {
							current_layer_ids.insert(cell->node()->getId());
							agent->model()->link(agent, cell, layer_id);
						}
				};
			private:
				graph::DistributedId current_location_id;
				const api::model::Range<CellType>* mobility_range;
				const api::model::Range<CellType>* perception_range;

			protected:
				SpatialAgentBase(
						const api::model::Range<CellType>& mobility_range,
						const api::model::Range<CellType>& perception_range) :
					mobility_range(&mobility_range),
					perception_range(&perception_range) {}

				void updateLocation(api::model::Cell* cell);

				CellType* locationCell() const override;
				void handleNewMove() override;
				void handleNewPerceive() override;

				using api::model::SpatialAgent::moveTo;
				void moveTo(graph::DistributedId id) override;

			public:
				void initLocation(api::model::Cell* cell) override {
					this->moveTo(cell);
				}

				fpmas::graph::DistributedId locationId() const override {
					return current_location_id;
				}
		};

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::updateLocation(api::model::Cell* cell) {
			// Links to new location
			this->model()->link(this, cell, SpatialModelLayers::NEW_LOCATION);

			// Unlinks obsolete location
			auto location = this->template outNeighbors<api::model::Cell>(
					SpatialModelLayers::LOCATION);
			if(location.count() > 0)
				this->model()->unlink(location.at(0).edge());

			// Unlinks obsolete mobility field
			for(auto cell : this->template outNeighbors<api::model::Cell>(
						SpatialModelLayers::MOVE)) {
				this->model()->unlink(cell.edge());
			}

			// Unlinks obsolete perception field
			for(auto agent : this->template outNeighbors<api::model::Agent>(
						SpatialModelLayers::PERCEPTION))
				this->model()->unlink(agent.edge());

			// Adds the NEW_LOCATION to the mobility/perceptions fields
			// depending on the current ranges
			if(auto _cell = dynamic_cast<CellType*>(cell)) {
				if(this->mobility_range->contains(_cell, _cell))
					this->model()->link(this, _cell, SpatialModelLayers::MOVE);
				if(this->perception_range->contains(_cell, _cell)) {
					this->model()->link(this, _cell, SpatialModelLayers::PERCEIVE);
				}
			}

			this->current_location_id = cell->node()->getId();
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

			for(auto cell : this->template outNeighbors<CellType>(
						SpatialModelLayers::NEW_MOVE)) {
				if(!move_layer.contains(cell)
						&& this->mobility_range->contains(this->locationCell(), cell))
					move_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewPerceive() {
			CurrentOutLayer perceive_layer(this, SpatialModelLayers::PERCEIVE);

			for(auto cell : this->template outNeighbors<CellType>(
						SpatialModelLayers::NEW_PERCEIVE)) {
				if(!perceive_layer.contains(cell)
						&& this->perception_range->contains(this->locationCell(), cell))
					perceive_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}
	
	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::moveTo(graph::DistributedId cell_id) {
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

	template<typename AgentType, typename CellType, typename Derived = AgentType>
		class SpatialAgent :
			public SpatialAgentBase<AgentType, CellType, Derived> {
				protected:
					using SpatialAgentBase<AgentType, CellType, Derived>::moveTo;
					using SpatialAgentBase<AgentType, CellType, Derived>::SpatialAgentBase;

					void moveTo(api::model::Cell* cell) override {
						this->updateLocation(cell);
					}
			};

	template<typename AgentType>
		class DefaultSpatialAgentFactory : public api::model::SpatialAgentFactory {
			public:
				AgentType* build() override {
					return new AgentType;
				}
		};

	class SpatialAgentBuilder {
		private:
			fpmas::api::model::SpatialModel& spatial_model;
		public:
			SpatialAgentBuilder(
					fpmas::api::model::SpatialModel& spatial_model
					) : spatial_model(spatial_model) {}

			void build(
					api::model::GroupList groups,
					api::model::SpatialAgentFactory& factory,
					api::model::SpatialAgentMapping& agent_counts,
					std::vector<api::model::Cell*> cells);
	};
}}

namespace nlohmann {
	template<typename AgentType, typename CellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				j[1] = ptr->current_location_id;
			}

			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				derived_ptr->current_location_id = j[1].get<fpmas::graph::DistributedId>();
				return derived_ptr.get();
			}
		};

}

#endif
