#include "grid_load_balancing.h"

namespace fpmas { namespace model {
	TreeProcessMapping::TreeProcessMapping() {
		root = new Node;
		root->node_type = LEAF;
		root->process = 0;
	}

	TreeProcessMapping::TreeProcessMapping(
			DiscreteCoordinate width, DiscreteCoordinate height,
			api::communication::MpiCommunicator& comm
			) {
		root = new Node;
		root->origin = {0, 0};
		root->width = width;
		root->height = height;

		std::list<Node*> leafs;
		leafs.push_back(root);
		while(leafs.size() != comm.getSize()) {
			split_node(leafs);
		}
		int process = 0;
		for(auto leaf : leafs) {
			leaf->node_type = LEAF;
			leaf->process = process;
			process++;
		}
	}

	void TreeProcessMapping::split_node(std::list<Node*>& leafs) {
		Node* node = leafs.front();
		if(node->width > node->height) {
			node->node_type = VERTICAL_FRONTIER;
			node->value = node->origin.x + node->width / 2;

			Node* left = new Node;
			left->origin = node->origin;
			left->height = node->height;
			left->width = node->width / 2;
			node->childs.push_back(left);
			leafs.push_back(left);

			Node* right = new Node;
			right->origin = {node->value, node->origin.y};
			right->height = node->height;
			right->width = node->width - left->width;
			node->childs.push_back(right);
			leafs.push_back(right);
		} else {
			node->node_type = HORIZONTAL_FRONTIER;
			node->value = node->origin.y + node->height / 2;

			Node* bottom = new Node;
			bottom->origin = node->origin;
			bottom->height = node->height / 2;
			bottom->width = node->width;
			node->childs.push_back(bottom);
			leafs.push_back(bottom);

			Node* top = new Node;
			top->origin = {node->origin.x, node->value};
			top->height = node->height - bottom->height;
			top->width = node->width;
			node->childs.push_back(top);
			leafs.push_back(top);
		}
		leafs.pop_front();
	}

	void TreeProcessMapping::delete_node(Node* node) {
		for(auto child : node->childs)
			delete_node(child);
		delete node;
	}

	TreeProcessMapping::~TreeProcessMapping() {
		delete_node(root);
	}

	int TreeProcessMapping::process(DiscretePoint point, Node* node) const {
		switch(node->node_type) {
			case LEAF:
				return node->process;
			case HORIZONTAL_FRONTIER:
				if(point.y < node->value) {
					return process(point, node->childs[0]);
				} else {
					return process(point, node->childs[1]);
				}
			case VERTICAL_FRONTIER:
				if(point.x < node->value) {
					return process(point, node->childs[0]);
				} else {
					return process(point, node->childs[1]);
				}
		}
		return -1;
	}

	int TreeProcessMapping::process(DiscretePoint point) const {
		return process(point, root);
	}

	StripProcessMapping::StripProcessMapping(
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

	int StripProcessMapping::process(DiscretePoint point) const {
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
			) :
		default_process_mapping(width, height, comm),
		grid_process_mapping(default_process_mapping) {
		};

	GridLoadBalancing::GridLoadBalancing(
			const GridProcessMapping& grid_process_mapping)
		: grid_process_mapping(grid_process_mapping) {
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
				} else if (api::model::GridAgentBase* grid_agent
						= dynamic_cast<api::model::GridAgentBase*>(node.second->data().get())) {
					partition[node.first] = grid_process_mapping.process(grid_agent->locationPoint());
				}
			}
		} else {
			// Optimized repartitioning, assuming cells are already assigned to
			// processes
			for(auto node : nodes) {
				auto location_edges = node.second->getOutgoingEdges(fpmas::api::model::LOCATION);
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
						partition[node.first] = cell_node->location();
					}
				}
			}
		}
		return partition;
	}


}}
