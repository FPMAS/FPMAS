#include "spatial_model.h"


namespace fpmas {
	namespace model {
		std::vector<api::model::Cell*> CellBase::neighborhood() {
			auto neighbors = this->outNeighbors<api::model::Cell>(SpatialModelLayers::NEIGHBOR_CELL);
			return {neighbors.begin(), neighbors.end()};
		}

		void CellBase::updateLocation(api::model::Neighbor<api::model::Agent>& agent) {
			FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
					"%s Setting this Cell as %s location.",
					FPMAS_C_STR(this->node()->getId()),
					FPMAS_C_STR(agent->node()->getId()));
			this->model()->link(agent, this, SpatialModelLayers::LOCATION);
			this->model()->unlink(agent.edge());
		}

		void CellBase::growMobilityRange(api::model::Agent* agent) {
			for(auto cell : this->neighborhood())
				this->model()->link(agent, cell, SpatialModelLayers::NEW_MOVE);
		}

		void CellBase::growPerceptionRange(api::model::Agent* agent) {
			for(auto cell : this->neighborhood())
				this->model()->link(agent, cell, SpatialModelLayers::NEW_PERCEIVE);
		}

		void CellBase::handleNewLocation() {
			FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
					"%s Updating ranges...",
					FPMAS_C_STR(this->node()->getId()));
			for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::NEW_LOCATION)) {
				updateLocation(agent);
				growMobilityRange(agent);
				growPerceptionRange(agent);

				// If this node is to be linked in the MOVE or PERCEPTION
				// field, this has already been done by
				// LocatedAgent::moveToCell(), so this current location does
				// not need to be explored for this agent.
				move_flags.insert(agent->node()->getId());
				perception_flags.insert(agent->node()->getId());
			}
		}

		void CellBase::handleMove() {
			for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::MOVE)) {
				if(move_flags.count(agent->node()->getId()) == 0) {
					growMobilityRange(agent);
					move_flags.insert(agent->node()->getId());
				}
			}
		}

		void CellBase::handlePerceive() {
			for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::PERCEIVE)) {
				if(perception_flags.count(agent->node()->getId()) == 0) {
					growPerceptionRange(agent);
					perception_flags.insert(agent->node()->getId());
				}
			}
		}

		void CellBase::updatePerceptions() {
			FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
					"%s Updating perceptions...",
					FPMAS_C_STR(this->node()->getId()));
		
			move_flags.clear();
			perception_flags.clear();
			for(auto agent : this->inNeighbors<api::model::Agent>(
						SpatialModelLayers::PERCEIVE)) {
				for(auto perceived_agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::LOCATION))
					if(perceived_agent->node()->getId() != agent->node()->getId())
						this->model()->link(agent, perceived_agent, SpatialModelLayers::PERCEPTION);
				this->model()->unlink(agent.edge());
			}
		}

		SpatialModelBase::SpatialModelBase(
				unsigned int max_mobility_range,
				unsigned int max_perception_range) :
			max_mobility_range(max_mobility_range),
			max_perception_range(max_perception_range)
		{}

		void SpatialModelBase::add(api::model::SpatialAgentBase* agent) {
			spatial_agent_group->add(agent);
		}

		void SpatialModelBase::add(api::model::Cell* cell) {
			cell_behavior_group->add(cell);
			update_perceptions_group->add(cell);
		}

		std::vector<api::model::Cell*> SpatialModelBase::cells() {
			std::vector<api::model::Cell*> cells;
			for(auto agent : cell_behavior_group->localAgents())
				cells.push_back(dynamic_cast<api::model::Cell*>(agent));
			return cells;
		}

		api::scheduler::JobList SpatialModelBase::initLocationAlgorithm() {
			api::scheduler::JobList _jobs;

			for(unsigned int i = 0; i < std::max(max_perception_range, max_mobility_range); i++) {
				_jobs.push_back(cell_behavior_group->job());
				_jobs.push_back(spatial_agent_group->job());
			}

			_jobs.push_back(update_perceptions_group->job());
			return _jobs;
		}

		api::scheduler::JobList SpatialModelBase::distributedMoveAlgorithm(
					const AgentGroup& movable_agents
					) {
			api::scheduler::JobList _jobs;

			_jobs.push_back(movable_agents.job());

			api::scheduler::JobList update_location_algorithm
				= initLocationAlgorithm();
			for(auto job : update_location_algorithm)
				_jobs.push_back(job);

			return _jobs;
		}

		void SpatialAgentBuilder::build(
				api::model::AgentGroup& group,
				api::model::SpatialAgentFactory& factory,
				api::model::SpatialAgentMapping& agent_counts,
				std::vector<api::model::Cell*> local_cells) {

			for(auto cell : local_cells)
				for(std::size_t i = 0; i < agent_counts.countAt(cell); i++) {
					auto agent = factory.build();
					group.add(agent);
					agent->initLocation(cell);
				}

			spatial_model.runtime().execute(spatial_model.initLocationAlgorithm());
		}
	}
}
