#include "spatial_model.h"


namespace fpmas {
	namespace model {

		MoveAgentGroup::MoveAgentGroup(
				api::model::GroupId group_id,
				api::model::AgentGraph& agent_graph,
				api::model::SpatialModel& model) :
			AgentGroupBase(group_id, agent_graph), model(model) {

			}

		MoveAgentGroup::MoveAgentGroup(
				api::model::GroupId group_id,
				api::model::AgentGraph& agent_graph,
				const api::model::Behavior& behavior,
				api::model::SpatialModel& model) :
			AgentGroupBase(group_id, agent_graph, behavior), model(model) {
			}

		api::scheduler::JobList MoveAgentGroup::jobs() const {
			api::scheduler::JobList job_list;
			job_list.push_back(this->job());
			for(auto job : model.distributedMoveAlgorithm())
				job_list.push_back(job);
			return job_list;
		}

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

		api::model::AgentGroup& SpatialModelBase::buildMoveGroup(
				api::model::GroupId id, const api::model::Behavior& behavior) {
			auto* group = new MoveAgentGroup(id, this->graph(), behavior, *this);
			this->insert(id, group);
			return *group;
		}

		api::scheduler::JobList SpatialModelBase::distributedMoveAlgorithm() {
			api::scheduler::JobList _jobs;

			for(unsigned int i = 0; i < std::max(max_perception_range, max_mobility_range); i++) {
				_jobs.push_back(cell_behavior_group->agentExecutionJob());
				_jobs.push_back(spatial_agent_group->agentExecutionJob());
			}

			_jobs.push_back(update_perceptions_group->agentExecutionJob());
			return _jobs;
		}
	}
}
