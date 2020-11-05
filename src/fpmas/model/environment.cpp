#include "environment.h"


namespace fpmas {
	namespace model {
		/*
		 * CellBase implementation.
		 */

		std::vector<api::model::Cell*> CellBase::neighborhood() {
			auto neighbors = this->outNeighbors<api::model::Cell>(EnvironmentLayers::NEIGHBOR_CELL);
			return {neighbors.begin(), neighbors.end()};
		}

		void CellBase::updateLocation(api::model::Neighbor<api::model::LocatedAgent>& agent) {
			this->model()->link(agent, this, EnvironmentLayers::LOCATION);
			this->model()->unlink(agent.edge());
			if(agent->perceptionRange().contains(this))
				this->model()->link(agent, this, EnvironmentLayers::PERCEIVE);
			if(agent->mobilityRange().contains(this)) {
				this->model()->link(agent, this, EnvironmentLayers::MOVE);
			}
		}

		void CellBase::growMobilityRange(api::model::LocatedAgent* agent) {
			for(auto cell : this->neighborhood())
				if(agent->mobilityRange().contains(cell)) {
					this->model()->link(agent, cell, EnvironmentLayers::NEW_MOVE);
				}
		}

		void CellBase::growPerceptionRange(api::model::LocatedAgent* agent) {
			for(auto cell : this->neighborhood())
				if(agent->perceptionRange().contains(cell))
					this->model()->link(agent, cell, EnvironmentLayers::NEW_PERCEIVE);
		}

		void CellBase::updateRanges() {
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_LOCATION)) {
				updateLocation(agent);
				growMobilityRange(agent);
				growPerceptionRange(agent);
				move_flags.insert(agent->node()->getId());
				perception_flags.insert(agent->node()->getId());
			}
			for(auto agent: this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_MOVE)) {
				if(move_flags.count(agent->node()->getId()) == 0) {
					this->model()->link(agent, this, EnvironmentLayers::MOVE);
					growMobilityRange(agent);
					move_flags.insert(agent->node()->getId());
				}
				this->model()->unlink(agent.edge());
			}
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_PERCEIVE)) {
				if(perception_flags.count(agent->node()->getId()) == 0) {
					this->model()->link(agent, this, EnvironmentLayers::PERCEIVE);
					growPerceptionRange(agent);
					perception_flags.insert(agent->node()->getId());
				}
				this->model()->unlink(agent.edge());
			}
		}

		void CellBase::updatePerceptions() {
			move_flags.clear();
			perception_flags.clear();
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_MOVE))
				this->model()->unlink(agent.edge());
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_PERCEIVE))
				this->model()->unlink(agent.edge());

			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::PERCEIVE)) {
				for(auto perceived_agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::LOCATION))
					if(perceived_agent->node()->getId() != agent->node()->getId())
						this->model()->link(agent, perceived_agent, EnvironmentLayers::PERCEPTION);
				this->model()->unlink(agent.edge());
			}
		}

		/*
		 * Environment implementation.
		 */

		Environment::Environment(api::model::Model& model) :
			update_ranges_group(model.buildGroup(
						CELL_UPDATE_RANGES, update_ranges_behavior)),
			update_perceptions_group(model.buildGroup(
						CELL_UPDATE_PERCEPTIONS, update_perceptions_behavior)) {
			}

		void Environment::add(std::vector<api::model::Cell *> cells) {
			for(api::model::Cell* cell : cells) {
				update_ranges_group.add(cell);
				update_perceptions_group.add(cell);
			}
		}

		std::vector<fpmas::api::model::Cell*> Environment::localCells() {
			std::vector<fpmas::api::model::Cell*> cells;
			for(auto agent : update_ranges_group.localAgents())
				cells.push_back(dynamic_cast<fpmas::api::model::Cell*>(agent));
			return cells;
		}

		api::scheduler::JobList Environment::initLocationAlgorithm(
				unsigned int max_perception_range,
				unsigned int max_mobility_range) {
			api::scheduler::JobList _jobs;

			for(unsigned int i = 0; i <= std::max(max_perception_range, max_mobility_range); i++)
				_jobs.push_back(update_ranges_group.job());

			_jobs.push_back(update_perceptions_group.job());
			return _jobs;
		}

		api::scheduler::JobList Environment::distributedMoveAlgorithm(
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

	}
	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const api::model::EnvironmentLayers& layer) {
			switch(layer) {
				case NEIGHBOR_CELL:
					os << "NEIGHBOR_CELL";
					break;
				case LOCATION:
					os << "LOCATION";
					break;
				case MOVE:
					os << "MOVE";
					break;
				case PERCEIVE:
					os << "PERCEIVE";
					break;
				case PERCEPTION:
					os << "PERCEPTION";
					break;
				case NEW_LOCATION:
					os << "NEW_LOCATION";
					break;
				case NEW_MOVE:
					os << "NEW_MOVE";
					break;
				case NEW_PERCEIVE:
					os << "NEW_PERCEIVE";
					break;
			}
			return os;
		}
	}}

}
