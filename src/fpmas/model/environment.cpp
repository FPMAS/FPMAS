#include "environment.h"


namespace fpmas {
	namespace model {
		Neighbors<api::model::Cell> CellBase::neighborCells() const {
			return this->outNeighbors<api::model::Cell>(EnvironmentLayers::NEIGHBOR_CELL);
		}

		void CellBase::updateLocation(api::model::Neighbor<api::model::LocatedAgent>& agent) {
			this->model()->link(agent, this, EnvironmentLayers::LOCATION);
			this->model()->unlink(agent.edge());
			if(agent->isInPerceptionRange(this))
				this->model()->link(agent, this, EnvironmentLayers::PERCEIVE);
			if(agent->isInMobilityRange(this))
				this->model()->link(agent, this, EnvironmentLayers::MOVE);
		}

		void CellBase::growMobilityRange(api::model::LocatedAgent* agent) {
			for(auto cell : this->neighborCells())
				if(agent->isInMobilityRange(cell)) {
					this->model()->link(agent, cell, EnvironmentLayers::NEW_MOVE);
				}
		}

		void CellBase::growPerceptionRange(api::model::LocatedAgent* agent) {
			for(auto cell : this->neighborCells())
				if(agent->isInPerceptionRange(cell))
					this->model()->link(agent, cell, EnvironmentLayers::NEW_PERCEIVE);
		}

		void CellBase::updateRanges() {
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_LOCATION)) {
				updateLocation(agent);
				growMobilityRange(agent);
				growPerceptionRange(agent);
			}
			for(auto agent: this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_MOVE)) {
				this->model()->link(agent, this, EnvironmentLayers::MOVE);
				this->model()->unlink(agent.edge());
				growMobilityRange(agent);
			}
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::NEW_PERCEIVE)) {
				this->model()->link(agent, this, EnvironmentLayers::PERCEIVE);
				this->model()->unlink(agent.edge());
				growPerceptionRange(agent);
			}
		}

		void CellBase::updatePerceptions() {
			for(auto agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::PERCEIVE)) {
				for(auto perceived_agent : this->inNeighbors<api::model::LocatedAgent>(EnvironmentLayers::LOCATION))
					this->model()->link(agent, perceived_agent, EnvironmentLayers::PERCEIVE);
				this->model()->unlink(agent.edge());
			}
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
