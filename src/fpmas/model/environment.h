#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "fpmas/api/model/environment.h"
#include "model.h"
#include "serializer.h"

namespace fpmas {
	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const api::model::EnvironmentLayers& layer);
	}}

	namespace model {
	using api::model::EnvironmentLayers;

	class CellBase : public api::model::Cell, public NeighborsAccess {
		private:
			std::set<fpmas::api::graph::DistributedId> move_flags;
			std::set<fpmas::api::graph::DistributedId> perception_flags;

		protected:
			void updateLocation(api::model::Neighbor<api::model::LocatedAgent>& agent) override;
			void growMobilityRange(api::model::LocatedAgent* agent) override;
			void growPerceptionRange(api::model::LocatedAgent* agent) override;

		public:
			std::vector<api::model::Cell*> neighborhood() override;
			void updateRanges() override;
			void updatePerceptions() override;

	};

	template<typename CellImpl>
	class Cell : public CellBase, public AgentBase<CellImpl> {

	};

	class DefaultCell : public Cell<DefaultCell> {};

	enum CellGroups : api::model::GroupId {
		CELL_UPDATE_RANGES = -1,
		CELL_UPDATE_PERCEPTIONS = -2
	};

	class Environment : public api::model::Environment {
		private:
			Behavior<api::model::Cell> update_ranges {
				&api::model::Cell::updateRanges};
			Behavior<api::model::Cell> update_perceptions {
				&api::model::Cell::updatePerceptions};

			AgentGroup& update_ranges_group;
			Behavior<api::model::Cell> update_ranges_behavior {
				&api::model::Cell::updateRanges};
			AgentGroup& update_perceptions_group;
			Behavior<api::model::Cell> update_perceptions_behavior {
				&api::model::Cell::updatePerceptions};

		public:
			Environment(api::model::Model& model);

			void add(std::vector<api::model::Cell*> cells) override;
			std::vector<api::model::Cell*> localCells() override;

			api::scheduler::JobList initLocationAlgorithm(
					unsigned int max_perception_range,
					unsigned int max_mobility_range) override;

			api::scheduler::JobList distributedMoveAlgorithm(
					const AgentGroup& movable_agents,
					unsigned int max_perception_range,
					unsigned int max_mobility_range) override;

	};

	template<typename AgentType>
	class LocatedAgent :
		public api::model::LocatedAgent, public model::AgentBase<AgentType> {
			public:
				void moveToCell(api::model::Cell* cell) override;
				api::model::Cell* location() override;
	};

	template<typename AgentType>
		void LocatedAgent<AgentType>::moveToCell(api::model::Cell* cell) {
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
		}

	template<typename AgentType>
		api::model::Cell* LocatedAgent<AgentType>::location() {
			auto location = this->template outNeighbors<api::model::Cell>(
					EnvironmentLayers::LOCATION);
			if(location.count() > 0)
				return location.at(0);
			return nullptr;
		}

}}

FPMAS_DEFAULT_JSON(fpmas::model::DefaultCell)
	
#endif
