#include "cell_load_balancing.h"
#include "fpmas/communication/communication.h"

namespace fpmas { namespace model {

	api::graph::PartitionMap CellLoadBalancing::balance(
			api::graph::NodeMap<api::model::AgentPtr> nodes
			) {
		return this->balance(nodes, api::graph::PARTITION);
	}

	api::graph::PartitionMap CellLoadBalancing::balance(
			api::graph::NodeMap<api::model::AgentPtr> nodes,
			api::graph::PartitionMode partition_mode) {
		// Original cell weights backup
		std::vector<std::pair<api::graph::DistributedNode<api::model::AgentPtr>*, float>> cell_weights;
		// Local cells
		api::graph::NodeMap<api::model::AgentPtr> cells;

		for(auto node : nodes) {
			if(auto cell = dynamic_cast<api::model::Cell*>(node.second->data().get())) {
				cell_weights.push_back({node.second, node.second->getWeight()});
				// New weight = cell weight + weights of all agents located in
				// the cell
				float new_weight = cell->node()->getWeight();
				for(auto agent : cell->node()->getIncomingEdges(api::model::LOCATION))
					new_weight += agent->getWeight();
				cell->node()->setWeight(new_weight);

				cells.insert(node);
			}
		}

		// Apply lb algorithm to the cell network
		api::graph::PartitionMap partition
			= cell_lb.balance(cells, partition_mode);

		// Even if the partitionned cells are LOCAL, agents located in each
		// cell might be DISTANT. In consequence, it is necessary to send the
		// new location of DISTANT agents to the owner process, where the agent
		// is LOCAL.
		std::unordered_map<int, std::vector<std::pair<DistributedId, int>>> distant_agents_location;

		for(auto cell : cells) {
			for(auto agent_edge : cell.second->getIncomingEdges(api::model::LOCATION)) {
				auto agent = agent_edge->getSourceNode();
				// By default, the agent location is the current cell location
				int agent_location = cell.second->location();
				auto location_it = partition.find(cell.first);
				if(location_it != partition.end())
					// If a new location has been assigned to the cell, agent
					// location is this new location
					agent_location = location_it->second;
				// Nothing to do if the agent location has not changed
				if(agent_location != agent->location()) {
					if(agent->state() == api::graph::LOCAL) {
						// The agent is LOCAL, so its new location can directly
						// be added to the local partition
						partition[agent->getId()] = agent_location;
					} else {
						// Else, store the new location in
						// distant_agents_location to send it to the owner
						// process
						distant_agents_location[agent->location()].push_back(
								{agent->getId(), agent_location}
								);
					}
				}
			}
		}

		fpmas::communication::TypedMpi<std::vector<std::pair<DistributedId, int>>> mpi(comm);
		std::unordered_map<int, std::vector<std::pair<DistributedId, int>>> imported_agents_location = mpi.allToAll(distant_agents_location);
		for(auto item : imported_agents_location)
			for(auto agent_location : item.second)
				partition[agent_location.first] = agent_location.second;
		for(auto item : cell_weights)
			// Restores original weight
			item.first->setWeight(item.second);

		return partition;
	}
}}
