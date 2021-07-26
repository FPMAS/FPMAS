#include "grid_load_balancing.h"

namespace fpmas { namespace model {
	GridProcessMapping::GridProcessMapping(
			DiscreteCoordinate width, DiscreteCoordinate height,
			api::communication::MpiCommunicator& comm
			) : mode(width < height ? Mode::VERTICAL : Mode::HORIZONTAL) {
		switch(mode) {
			case VERTICAL:
				for(int i = 0; i < comm.getSize(); i++)
					process_bounds[i*height / comm.getSize()] = i;
				break;
			case HORIZONTAL:
				for(int i = 0; i < comm.getSize(); i++)
					process_bounds[i*width / comm.getSize()] = i;
		}
	}

	int GridProcessMapping::process(DiscretePoint point) {
		int process;
		switch(mode) {
			case VERTICAL:
				process = (--process_bounds.upper_bound(point.y))->second;
				break;
			case HORIZONTAL:
				process = (--process_bounds.upper_bound(point.x))->second;
				break;
			default:
				process=-1;
		}
		return process;
	}
	GridLoadBalancing::GridLoadBalancing(
			DiscreteCoordinate width, DiscreteCoordinate height,
			api::communication::MpiCommunicator& comm
			) : grid_process_mapping(width, height, comm) {};

	void GridLoadBalancing::assignAgentToPartition(
			api::model::AgentNode* node, api::graph::PartitionMap& partition) {
		auto location_edges = node->getOutgoingEdges(fpmas::api::model::LOCATION);
		if(location_edges.size() > 0) {
			auto cell_node = location_edges[0]->getTargetNode();
			// The agent represented by node is located, i.e. its a
			// spatial agent.
			// In consequence, cells are automatically excluded without
			// the need for a dynamic cast
			if(cell_node->state() == api::graph::DISTANT) {
				// An agent must be migrated only if its location is
				// DISTANT
				// Migrates the agent to the process that owns its
				// location
				partition[node->getId()] = cell_node->location();
			}
		}
	}
	api::graph::PartitionMap GridLoadBalancing::balance(
			api::graph::NodeMap<api::model::AgentPtr> nodes) {
		return balance(nodes, api::graph::PARTITION);
	}

	api::graph::PartitionMap GridLoadBalancing::balance(
			api::graph::NodeMap<api::model::AgentPtr> nodes,
			api::graph::PartitionMode partition_mode
			) {
		api::graph::PartitionMap partition;
		if(partition_mode == api::graph::PARTITION) {
			// Initial partitioning: needs to assign cells to processes
			for(auto node : nodes) {
				if(api::model::GridCell* grid_cell
						= dynamic_cast<api::model::GridCell*>(node.second->data().get())) {
					partition[node.first] = grid_process_mapping.process(grid_cell->location());
				} else {
					assignAgentToPartition(node.second, partition);
				}
			}
		} else {
			// Optimized repartitioning, assuming cells are already assigned to
			// processes
			for(auto node : nodes) {
				assignAgentToPartition(node.second, partition);
			}
		}
		return partition;
	}


}}
