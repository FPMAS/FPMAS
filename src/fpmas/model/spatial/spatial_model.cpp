#include "spatial_model.h"
#include "fpmas/utils/functional.h"


namespace fpmas { namespace model {

	bool is_agent_in_group(api::model::Agent* agent, api::model::AgentGroup& group) {
		std::vector<fpmas::api::model::GroupId> group_ids = agent->groupIds();
		auto result = std::find(group_ids.begin(), group_ids.end(), group.groupId());
		return result != group_ids.end();
	}

	std::vector<api::model::Cell*> CellBase::successors() {
		auto neighbors = this->outNeighbors<api::model::Cell>(SpatialModelLayers::CELL_SUCCESSOR);
		return {neighbors.begin(), neighbors.end()};
	}

	/**
	 * Replaces the NEW_LOCATION link associated to `agent` by a LOCATION
	 * link.
	 */
	void CellBase::updateLocation(
			api::model::Agent* agent,
			api::model::AgentEdge* new_location_edge
			) {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Setting this Cell as %s location.",
				FPMAS_C_STR(this->node()->getId()),
				FPMAS_C_STR(agent->node()->getId()));
		if(new_location_edge->state() == api::graph::LOCAL) {
			agent->model()->graph().switchLayer(
					new_location_edge, SpatialModelLayers::LOCATION
					);
		} else {
			this->model()->link(agent, this, SpatialModelLayers::LOCATION);
			this->model()->unlink(new_location_edge);
		}
	}
	 
	void CellBase::handleNewLocation() {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Updating ranges...",
				FPMAS_C_STR(this->node()->getId()));

		for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::NEW_LOCATION)) {
			AgentPtr& agent = agent_edge->getSourceNode()->data();
			updateLocation(agent, agent_edge);
			growMobilityField(agent);
			growPerceptionField(agent);

			// If this node is to be linked in the MOVE or PERCEPTION
			// field, this has already been done by
			// LocatedAgent::moveToCell(), so this current location does
			// not need to be explored for this agent.
			move_flags.insert(agent->node()->getId());
			perception_flags.insert(agent->node()->getId());
		}
	}

	void CellBase::handleMove() {
		for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::MOVE)) {
			auto agent = agent_edge->getSourceNode();
			if(
					// The agent updated its location since the last
					// DistributedMoveAlgorithm execution
					no_move_flags.count(agent->getId()) == 0 &&
					// The current cell was not explored yet
					move_flags.count(agent->getId()) == 0) {
				growMobilityField(agent->data());
				move_flags.insert(agent->getId());
			}
		}
	}

	void CellBase::handlePerceive() {
		for(auto agent_edge : this->node()->getIncomingEdges(SpatialModelLayers::PERCEIVE)) {
			auto agent = agent_edge->getSourceNode();
			if(
					// The agent updated its location since the last
					// DistributedMoveAlgorithm execution
					no_move_flags.count(agent->getId()) == 0 &&
					// The current cell was not explored yet
					perception_flags.count(agent->getId()) == 0) {
				growPerceptionField(agent->data());
				perception_flags.insert(agent->getId());
			}
		}
	}

	void CellBase::updatePerceptions(api::model::AgentGroup&) {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Updating perceptions...",
				FPMAS_C_STR(this->node()->getId()));

		move_flags.clear();
		perception_flags.clear();
		std::vector<api::model::AgentNode*> perceived_agents =
			this->node()->inNeighbors(SpatialModelLayers::LOCATION);
		for(auto agent : this->node()->inNeighbors(SpatialModelLayers::PERCEIVE)) {
			for(auto perceived_agent : perceived_agents)
				// Perceptions need to be updated only if either the perceivee
				// or the perceiver location has been updated since the last
				// DistributedMoveAlgorithm execution
				if(
						this->no_move_flags.count(agent->getId()) == 0 ||
						this->no_move_flags.count(perceived_agent->getId()) == 0
						)
					if(perceived_agent->getId() != agent->getId())
						this->model()->graph().link(
								agent, perceived_agent,
								SpatialModelLayers::PERCEPTION
								);
		}
		no_move_flags.clear();
	}
}}
