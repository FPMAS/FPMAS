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
		this->model()->link(agent, this, SpatialModelLayers::LOCATION);
		this->model()->unlink(new_location_edge);
	}

	/**
	 * Grows the current `agent`s mobility field, connecting it to
	 * successors() on the NEW_MOVE layer.
	 */
	void CellBase::growMobilityField(api::model::Agent* agent) {
		for(auto cell : this->bufferedSuccessors())
			this->model()->link(agent, cell, SpatialModelLayers::NEW_MOVE);
	}

	/**
	 * Grows the current `agent`s perception field, connecting it to
	 * successors() on the NEW_PERCEIVE layer.
	 */
	void CellBase::growPerceptionField(api::model::Agent* agent) {
		for(auto cell : this->bufferedSuccessors())
			this->model()->link(agent, cell, SpatialModelLayers::NEW_PERCEIVE);
	}

	bool CellBase::isAgentInGroup(api::model::Agent* agent, api::model::AgentGroup& group) {
		std::vector<fpmas::api::model::GroupId> group_ids = agent->groupIds();
		auto result = std::find(group_ids.begin(), group_ids.end(), group.groupId());
		return result != group_ids.end();
	}

	const std::vector<api::model::Cell*>& CellBase::bufferedSuccessors() {
		if(!init_successors) {
			this->successors_buffer = this->successors();
			this->raw_successors_buffer
				= this->node()->outNeighbors(SpatialModelLayers::CELL_SUCCESSOR);
			init_successors = true;
		}
		return this->successors_buffer;
	}

	void CellBase::init() {
		// This has no performance impact
		auto current_successors
			= this->node()->getOutgoingEdges(SpatialModelLayers::CELL_SUCCESSOR);
		// Checks if the currently buffered successors are stricly equal to the
		// current_successors. In this case, there is no need to update the
		// successors() list
		if(current_successors.size() == 0 ||
				current_successors.size() != raw_successors_buffer.size()) {
			init_successors = false;
		} else {
			auto it = current_successors.begin();
			auto raw_it = raw_successors_buffer.begin();
			while(it != current_successors.end() && (*it)->getTargetNode() == *raw_it) {
				it++;
				raw_it++;
			}
			if(it == current_successors.end()) {
				init_successors = true;
			} else {
				init_successors = false;
			}
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
