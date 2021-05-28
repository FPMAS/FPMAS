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
	void CellBase::updateLocation(Neighbor<api::model::Agent>& agent) {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Setting this Cell as %s location.",
				FPMAS_C_STR(this->node()->getId()),
				FPMAS_C_STR(agent->node()->getId()));
		this->model()->link(agent, this, SpatialModelLayers::LOCATION);
		this->model()->unlink(agent.edge());
	}

	/**
	 * Grows the current `agent`s mobility field, connecting it to
	 * successors() on the NEW_MOVE layer.
	 */
	void CellBase::growMobilityField(api::model::Agent* agent) {
		for(auto cell : this->successors())
			this->model()->link(agent, cell, SpatialModelLayers::NEW_MOVE);
	}

	/**
	 * Grows the current `agent`s perception field, connecting it to
	 * successors() on the NEW_PERCEIVE layer.
	 */
	void CellBase::growPerceptionField(api::model::Agent* agent) {
		for(auto cell : this->successors())
			this->model()->link(agent, cell, SpatialModelLayers::NEW_PERCEIVE);
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
		for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::NEW_LOCATION)) {
			updateLocation(agent);
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
		for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::MOVE)) {
			if(move_flags.count(agent->node()->getId()) == 0) {
				growMobilityField(agent);
				move_flags.insert(agent->node()->getId());
			}
		}
	}

	void CellBase::handlePerceive() {
		for(auto agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::PERCEIVE)) {
			if(perception_flags.count(agent->node()->getId()) == 0) {
				growPerceptionField(agent);
				perception_flags.insert(agent->node()->getId());
			}
		}
	}

	void CellBase::updatePerceptions(api::model::AgentGroup& group) {
		FPMAS_LOGD(this->model()->graph().getMpiCommunicator().getRank(), "[CELL]",
				"%s Updating perceptions...",
				FPMAS_C_STR(this->node()->getId()));

		move_flags.clear();
		perception_flags.clear();
		for(auto agent : this->inNeighbors<api::model::Agent>(
					SpatialModelLayers::PERCEIVE)) {
			for(auto perceived_agent : this->inNeighbors<api::model::Agent>(SpatialModelLayers::LOCATION))
				if(isAgentInGroup(agent, group) || isAgentInGroup(perceived_agent, group))
					if(perceived_agent->node()->getId() != agent->node()->getId())
						this->model()->link(agent, perceived_agent, SpatialModelLayers::PERCEPTION);
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
