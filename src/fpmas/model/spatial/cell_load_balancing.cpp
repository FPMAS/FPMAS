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
		std::vector<std::pair<api::graph::DistributedNode<api::model::AgentPtr>*, float>> cell_weights;
		api::graph::NodeMap<api::model::AgentPtr> cells;
		api::graph::NodeMap<api::model::AgentPtr> agents;
		for(auto node : nodes) {
			if(auto cell = dynamic_cast<api::model::Cell*>(node.second->data().get())) {
				cell_weights.push_back({node.second, node.second->getWeight()});
				cell->node()->setWeight(
						cell->node()->getWeight() +
						cell->node()->getIncomingEdges(api::model::LOCATION).size()
						);
				cells.insert(node);
			} else {
				agents.insert(node);
			}
		}

		// Apply lb algorithm to the cell network
		api::graph::PartitionMap partition
			= cell_lb.balance(cells, partition_mode);

		std::unordered_map<int, std::vector<std::pair<DistributedId, int>>> distant_agents_location;
		for(auto cell : cells) {
			for(auto agent_edge : cell.second->getIncomingEdges(api::model::LOCATION)) {
				auto agent = agent_edge->getSourceNode();
				int agent_location = partition.count(cell.first) > 0 ?
					partition[cell.first] :
					cell.second->location();
				if(agent_location != agent->location()) {
					if(agent->state() == api::graph::LOCAL) {
						partition[agent->getId()] = agent_location;
					} else {
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
