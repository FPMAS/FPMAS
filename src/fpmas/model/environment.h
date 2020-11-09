#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "fpmas/api/model/environment.h"
#include "model.h"
#include "serializer.h"

namespace fpmas {
	namespace model {
	using api::model::EnvironmentLayers;

	class CellBase : public api::model::Cell, public NeighborsAccess {
		private:
			std::set<fpmas::api::graph::DistributedId> move_flags;
			std::set<fpmas::api::graph::DistributedId> perception_flags;

		protected:
			void updateLocation(api::model::Neighbor<api::model::Agent>& agent);
			void growMobilityRange(api::model::Agent* agent);
			void growPerceptionRange(api::model::Agent* agent);

		public:
			std::vector<api::model::Cell*> neighborhood() override;
			void growRanges() override;
			void updatePerceptions() override;

	};

	template<typename CellImpl>
	class Cell : public CellBase, public AgentBase<CellImpl> {
	};

	class DefaultCell : public Cell<DefaultCell> {};

	enum CellGroups : api::model::GroupId {
		CELL_UPDATE_RANGES = -1,
		CELL_UPDATE_PERCEPTIONS = -2,
		AGENT_CROP_RANGES = -3,
	};

	template<typename CellType>
	class Environment : public api::model::Environment<CellType> {
		private:
			AgentGroup& update_ranges_group;
			Behavior<CellType> update_ranges_behavior {
				&CellType::growRanges};
			AgentGroup& crop_ranges_group;
			Behavior<api::model::LocatedAgent<CellType>> crop_ranges_behavior {
				&api::model::LocatedAgent<CellType>::cropRanges};
			AgentGroup& update_perceptions_group;
			Behavior<CellType> update_perceptions_behavior {
				&CellType::updatePerceptions};

		public:
			Environment(api::model::Model& model);

			void add(api::model::LocatedAgent<CellType>* agent) override;
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

		/*
		 * Environment implementation.
		 */

	template<typename CellType>
		Environment<CellType>::Environment(api::model::Model& model) :
			update_ranges_group(model.buildGroup(
						CELL_UPDATE_RANGES, update_ranges_behavior)),
			crop_ranges_group(model.buildGroup(
						AGENT_CROP_RANGES, crop_ranges_behavior)),
			update_perceptions_group(model.buildGroup(
						CELL_UPDATE_PERCEPTIONS, update_perceptions_behavior)) {
			}

	template<typename CellType>
		void Environment<CellType>::add(api::model::LocatedAgent<CellType>* agent) {
			crop_ranges_group.add(agent);
		}

	template<typename CellType>
		void Environment<CellType>::add(CellType* cell) {
			update_ranges_group.add(cell);
			update_perceptions_group.add(cell);
		}

	template<typename CellType>
		std::vector<CellType*> Environment<CellType>::localCells() {
			std::vector<CellType*> cells;
			for(auto agent : update_ranges_group.localAgents())
				cells.push_back(dynamic_cast<CellType*>(agent));
			return cells;
		}

	template<typename CellType>
		api::scheduler::JobList Environment<CellType>::initLocationAlgorithm(
				unsigned int max_perception_range,
				unsigned int max_mobility_range) {
			api::scheduler::JobList _jobs;

			for(unsigned int i = 0; i < std::max(max_perception_range, max_mobility_range); i++) {
				_jobs.push_back(update_ranges_group.job());
				_jobs.push_back(crop_ranges_group.job());
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


	template<typename AgentType, typename CellType>
	class LocatedAgent :
		public api::model::LocatedAgent<CellType>, public model::AgentBase<AgentType> {
			private:
				class CurrentOutLayer {
					private:
						LocatedAgent* agent;
						fpmas::graph::LayerId layer_id;
						std::set<DistributedId> current_layer_ids;

					public:
						CurrentOutLayer(LocatedAgent* agent, fpmas::graph::LayerId layer_id)
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
			public:
				void moveToCell(CellType* cell) override;
				CellType* location() override;

				void cropRanges() override;
	};

	template<typename AgentType, typename CellType>
		void LocatedAgent<AgentType, CellType>::moveToCell(CellType* cell) {
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
		}

	template<typename AgentType, typename CellType>
		CellType* LocatedAgent<AgentType, CellType>::location() {
			auto location = this->template outNeighbors<CellType>(
					EnvironmentLayers::LOCATION);
			if(location.count() > 0)
				return location.at(0);
			return nullptr;
		}

	template<typename AgentType, typename CellType>
		void LocatedAgent<AgentType, CellType>::cropRanges() {

			CurrentOutLayer move_layer(this, EnvironmentLayers::MOVE);

			for(auto cell : this->template outNeighbors<CellType>(
						EnvironmentLayers::NEW_MOVE)) {
				if(!move_layer.contains(cell)
						&& this->mobilityRange().contains(this->location(), cell))
					move_layer.link(cell);
				this->model()->unlink(cell.edge());
			}

			CurrentOutLayer perceive_layer(this, EnvironmentLayers::PERCEIVE);

			for(auto cell : this->template outNeighbors<CellType>(
						EnvironmentLayers::NEW_PERCEIVE)) {
				if(!perceive_layer.contains(cell)
						&& this->perceptionRange().contains(this->location(), cell))
					perceive_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}
}}

FPMAS_DEFAULT_JSON(fpmas::model::DefaultCell)
	
#endif
