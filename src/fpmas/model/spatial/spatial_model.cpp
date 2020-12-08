#include "spatial_model.h"


namespace fpmas {
	namespace model {
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
	}
}
