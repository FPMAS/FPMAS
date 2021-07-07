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

	api::graph::PartitionMap GridLoadBalancing::balance(
			api::graph::NodeMap<api::model::AgentPtr> nodes
			) {
		api::graph::PartitionMap partition;
		for(auto node : nodes) {
			if(api::model::GridCell* grid_cell
					= dynamic_cast<api::model::GridCell*>(node.second->data().get())) {
				partition[node.first] = grid_process_mapping.process(grid_cell->location());
					}
			else if(api::model::GridAgentBase* grid_agent
					= dynamic_cast<api::model::GridAgentBase*>(node.second->data().get())) {
				partition[node.first] = grid_process_mapping.process(grid_agent->locationPoint());
			}
		}
		return partition;
	}


}}
