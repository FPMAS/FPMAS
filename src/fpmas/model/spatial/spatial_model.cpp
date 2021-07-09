#include "spatial_model.h"
#include "fpmas/utils/functional.h"


namespace fpmas { namespace model {
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

	bool CellBase::isAgentInGroup(api::model::Agent* agent, api::model::AgentGroup& group) {
		std::vector<fpmas::api::model::GroupId> group_ids = agent->groupIds();
		auto result = std::find(group_ids.begin(), group_ids.end(), group.groupId());
		return result != group_ids.end();
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
		for(auto agent : this->node()->inNeighbors(SpatialModelLayers::MOVE)) {
			if(move_flags.count(agent->getId()) == 0) {
				growMobilityField(agent->data());
				move_flags.insert(agent->getId());
			}
		}
	}

	void CellBase::handlePerceive() {
		for(auto agent : this->node()->inNeighbors(SpatialModelLayers::PERCEIVE)) {
			if(perception_flags.count(agent->getId()) == 0) {
				growPerceptionField(agent->data());
				perception_flags.insert(agent->getId());
			}
		}
	}

	void CellBase::updatePerceptions(api::model::AgentGroup& group) {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Updating perceptions...",
				FPMAS_C_STR(this->node()->getId()));

		move_flags.clear();
		perception_flags.clear();
		std::vector<api::model::AgentNode*> perceived_agents =
			this->node()->inNeighbors(SpatialModelLayers::LOCATION);

		for(auto agent : this->node()->inNeighbors(SpatialModelLayers::PERCEIVE)) {
			for(auto perceived_agent : perceived_agents)
				if(
						isAgentInGroup(agent->data(), group) ||
						isAgentInGroup(perceived_agent->data(), group)
						)
					if(perceived_agent->getId() != agent->getId())
						this->model()->link(
								agent->data(), perceived_agent->data(),
								SpatialModelLayers::PERCEPTION
								);
		}
	}

	UniformAgentMapping::UniformAgentMapping(
			api::communication::MpiCommunicator& comm,
			api::model::AgentGroup& cell_group,
			std::size_t agent_count,
			std::uint_fast64_t seed
			) {
		// The same random generator is used on each process, so that the same
		// mapping is generated on any process
		fpmas::random::mt19937_64 rd(seed);
		std::vector<DistributedId> cell_ids;
		for(auto agent : cell_group.localAgents())
			cell_ids.push_back(agent->node()->getId());

		fpmas::communication::TypedMpi<decltype(cell_ids)> mpi(comm);
		cell_ids = fpmas::communication::all_reduce(
				mpi, cell_ids, utils::Concat()
				);

		fpmas::random::UniformIntDistribution<std::size_t> dist(0, cell_ids.size()-1);
		for(std::size_t i = 0; i < agent_count; i++)
			mapping[cell_ids[dist(rd)]]++;
	}

	std::size_t UniformAgentMapping::countAt(api::model::Cell* cell) {
		return mapping[cell->node()->getId()];
	}
}}
