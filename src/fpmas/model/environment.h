#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "fpmas/api/model/environment.h"
#include "fpmas/api/model/exceptions.h"
#include "model.h"
#include "serializer.h"

namespace fpmas {
	namespace model {
	using api::model::EnvironmentLayers;

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

	template<typename CellImpl>
	class Cell : public CellBase, public AgentBase<CellImpl> {
	};

	enum CellGroups : api::model::GroupId {
		CELL_UPDATE_RANGES = -1,
		CELL_UPDATE_PERCEPTIONS = -2,
		AGENT_CROP_RANGES = -3,
	};

	template<typename CellType>
	class Environment : public api::model::Environment<CellType> {
		private:
			AgentGroup& cell_behavior_group;
			Behavior<CellType> cell_behaviors {
				&CellType::handleNewLocation,
				&CellType::handleMove,
				&CellType::handlePerceive
			};
			AgentGroup& spatial_agent_group;
			Behavior<api::model::SpatialAgent<CellType>> spatial_agent_behaviors {
				&api::model::SpatialAgent<CellType>::handleNewMove,
				&api::model::SpatialAgent<CellType>::handleNewPerceive
			};
			AgentGroup& update_perceptions_group;
			Behavior<CellType> update_perceptions_behavior {
				&CellType::updatePerceptions
			};

		public:
			Environment(api::model::Model& model);

			void add(api::model::SpatialAgent<CellType>* agent) override;
			void add(CellType* cell) override;
			std::vector<CellType*> localCells() override;

			api::scheduler::JobList initLocationAlgorithm(
					unsigned int max_perception_range,
					unsigned int max_mobility_range) override;

			api::scheduler::JobList distributedMoveAlgorithm(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) override;

	};

	template<typename CellType>
		Environment<CellType>::Environment(api::model::Model& model) :
			cell_behavior_group(model.buildGroup(
						CELL_UPDATE_RANGES, cell_behaviors)),
			spatial_agent_group(model.buildGroup(
						AGENT_CROP_RANGES, spatial_agent_behaviors)),
			update_perceptions_group(model.buildGroup(
						CELL_UPDATE_PERCEPTIONS, update_perceptions_behavior)) {
			}

	template<typename CellType>
		void Environment<CellType>::add(api::model::SpatialAgent<CellType>* agent) {
			spatial_agent_group.add(agent);
		}

	template<typename CellType>
		void Environment<CellType>::add(CellType* cell) {
			cell_behavior_group.add(cell);
			update_perceptions_group.add(cell);
		}

	template<typename CellType>
		std::vector<CellType*> Environment<CellType>::localCells() {
			std::vector<CellType*> cells;
			for(auto agent : cell_behavior_group.localAgents())
				cells.push_back(dynamic_cast<CellType*>(agent));
			return cells;
		}

	template<typename CellType>
		api::scheduler::JobList Environment<CellType>::initLocationAlgorithm(
				unsigned int max_perception_range,
				unsigned int max_mobility_range) {
			api::scheduler::JobList _jobs;

			for(unsigned int i = 0; i < std::max(max_perception_range, max_mobility_range); i++) {
				_jobs.push_back(cell_behavior_group.job());
				_jobs.push_back(spatial_agent_group.job());
			}

			_jobs.push_back(update_perceptions_group.job());
			return _jobs;
		}

	template<typename CellType>
		api::scheduler::JobList Environment<CellType>::distributedMoveAlgorithm(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) {
			api::scheduler::JobList _jobs;

			_jobs.push_back(movable_agents.job());

			api::scheduler::JobList update_location_algorithm
				= initLocationAlgorithm(max_perception_range, max_mobility_range);
			for(auto job : update_location_algorithm)
				_jobs.push_back(job);

			return _jobs;
		}


	
	template<typename AgentType, typename CellType, typename Derived = AgentType>
	class SpatialAgentBase :
		public virtual api::model::SpatialAgent<CellType>, public model::AgentBase<AgentType> {
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

			protected:
				void updateLocation(CellType* cell);

				CellType* locationCell() const override;
				void handleNewMove() override;
				void handleNewPerceive() override;

				using api::model::SpatialAgent<CellType>::moveTo;
				void moveTo(graph::DistributedId id) override;

			public:
				void initLocation(CellType* cell) override {
					this->moveTo(cell);
				}

				fpmas::graph::DistributedId locationId() const override {
					return current_location_id;
				}
		};

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::updateLocation(CellType* cell) {
			// Links to new location
			this->model()->link(this, cell, EnvironmentLayers::NEW_LOCATION);

			// Unlinks obsolete location
			auto location = this->template outNeighbors<api::model::Cell>(
					EnvironmentLayers::LOCATION);
			if(location.count() > 0)
				this->model()->unlink(location.at(0).edge());

			// Unlinks obsolete mobility field
			for(auto cell : this->template outNeighbors<api::model::Cell>(
						EnvironmentLayers::MOVE)) {
				this->model()->unlink(cell.edge());
			}

			// Unlinks obsolete perception field
			for(auto agent : this->template outNeighbors<api::model::Agent>(
						EnvironmentLayers::PERCEPTION))
				this->model()->unlink(agent.edge());

			// Adds the NEW_LOCATION to the mobility/perceptions fields
			// depending on the current ranges
			if(this->mobilityRange().contains(cell, cell))
				this->model()->link(this, cell, EnvironmentLayers::MOVE);
			if(this->perceptionRange().contains(cell, cell)) {
				this->model()->link(this, cell, EnvironmentLayers::PERCEIVE);
			}

			this->current_location_id = cell->node()->getId();
		}

	template<typename AgentType, typename CellType, typename Derived>
		CellType* SpatialAgentBase<AgentType, CellType, Derived>::locationCell() const {
			auto location = this->template outNeighbors<CellType>(
					EnvironmentLayers::LOCATION);
			if(location.count() > 0)
				return location.at(0);
			return nullptr;
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewMove() {

			CurrentOutLayer move_layer(this, EnvironmentLayers::MOVE);

			for(auto cell : this->template outNeighbors<CellType>(
						EnvironmentLayers::NEW_MOVE)) {
				if(!move_layer.contains(cell)
						&& this->mobilityRange().contains(this->locationCell(), cell))
					move_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewPerceive() {
			CurrentOutLayer perceive_layer(this, EnvironmentLayers::PERCEIVE);

			for(auto cell : this->template outNeighbors<CellType>(
						EnvironmentLayers::NEW_PERCEIVE)) {
				if(!perceive_layer.contains(cell)
						&& this->perceptionRange().contains(this->locationCell(), cell))
					perceive_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}
	
	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::moveTo(graph::DistributedId cell_id) {
			bool found = false;
			auto mobility_field = this->template outNeighbors<CellType>(EnvironmentLayers::MOVE);
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

	template<typename AgentType, typename CellType>
		class SpatialAgent :
			public SpatialAgentBase<AgentType, CellType, AgentType> {
				protected:
					using SpatialAgentBase<AgentType, CellType, AgentType>::moveTo;

					void moveTo(CellType* cell) override {
						this->updateLocation(cell);
					}
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
