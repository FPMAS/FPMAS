#include "environment.h"


namespace fpmas {
	namespace model {
		Neighbors<api::model::Cell> CellBase::neighborCells() const {
			std::vector<Neighbor<api::model::Cell>> cells;
			for(auto raw_cell_edge : this->node()->getOutgoingEdges(EnvironmentLayers::NEIGHBOR_CELL))
				cells.push_back({&raw_cell_edge->getTargetNode()->data(), raw_cell_edge});

			return cells;
		}

		Neighbors<api::model::LocatedAgent> CellBase::newAgentsLocatedInThisCell() const {
			std::vector<Neighbor<api::model::LocatedAgent>> agents;
			for(auto raw_agent_edge : this->node()->getIncomingEdges(EnvironmentLayers::NEW_LOCATION))
				agents.push_back({&raw_agent_edge->getSourceNode()->data(), raw_agent_edge});
			return agents;
		}

		Neighbors<api::model::LocatedAgent> CellBase::newAgentsMovableToThisCell() const {
			std::vector<Neighbor<api::model::LocatedAgent>> agents;

			for(auto raw_agent_edge : this->node()->getIncomingEdges(EnvironmentLayers::NEW_MOVE))
				agents.push_back({&raw_agent_edge->getSourceNode()->data(), raw_agent_edge});
			return agents;
		}

		Neighbors<api::model::LocatedAgent> CellBase::newAgentsPerceivingThisCell() const {
			std::vector<Neighbor<api::model::LocatedAgent>> agents;

			for(auto raw_agent_edge : this->node()->getIncomingEdges(EnvironmentLayers::NEW_PERCEIVE))
				agents.push_back({&raw_agent_edge->getSourceNode()->data(), raw_agent_edge});
			return agents;
		}

		void CellBase::act() {
			for(auto agent : this->newAgentsLocatedInThisCell()) {
				updateLocation(agent);
				growMobilityRange(agent);
				growPerceptionRange(agent);
			}
			for(auto agent: this->newAgentsMovableToThisCell()) {
				this->model()->link(agent, this, EnvironmentLayers::MOVE);
				this->model()->unlink(agent.edge());
				growMobilityRange(agent);
			}
			for(auto agent : this->newAgentsPerceivingThisCell()) {
				this->model()->link(agent, this, EnvironmentLayers::PERCEIVE);
				this->model()->unlink(agent.edge());
				growPerceptionRange(agent);
			}
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
