#include "grid_load_balancing.h"
#include <cmath>
#include <limits>
#include "fpmas/communication/communication.h"

namespace fpmas { namespace model {

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
		while(leafs.size() != (std::size_t) comm.getSize()) {
			split_node(leafs);
		}
		process_grid_dimensions.resize(comm.getSize());
		int process = 0;
		for(auto leaf : leafs) {
			leaf->node_type = LEAF;
			leaf->process = process;
			process_grid_dimensions[process] = {
				leaf->origin,
				{leaf->origin.x + leaf->width, leaf->origin.y + leaf->height}
			};
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

	GridDimensions TreeProcessMapping::gridDimensions(int process) const {
		return process_grid_dimensions[process];
	}

	StripProcessMapping::StripProcessMapping(
			DiscreteCoordinate width, DiscreteCoordinate height,
			api::communication::MpiCommunicator& comm
			) :
		mode(width < height ? Mode::VERTICAL : Mode::HORIZONTAL),
		process_grid_dimensions(comm.getSize()) {
		switch(mode) {
			case VERTICAL:
				for(int i = 0; i < comm.getSize(); i++) {
					process_bounds[i*height / comm.getSize()] = i;
					process_grid_dimensions[i] = {
						{0, i*height/comm.getSize()},
						{width, (i+1)*height/comm.getSize()}
					};
				}
				break;
			case HORIZONTAL:
				for(int i = 0; i < comm.getSize(); i++) {
					process_bounds[i*width / comm.getSize()] = i;
					process_grid_dimensions[i] = {
						{i*width/comm.getSize(), 0},
						{(i+1)*width/comm.getSize(), height}
					};
				}
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


	GridDimensions StripProcessMapping::gridDimensions(int process) const {
		return process_grid_dimensions[process];
	}

	FastProcessMapping::FastProcessMapping(
			DiscreteCoordinate width, DiscreteCoordinate height,
			api::communication::MpiCommunicator& comm
			) :
		process_grid_dimensions(comm.getSize()),
		_n(-1), _p(-1), width(width), height(height) {
		int n = std::sqrt(comm.getSize());
		float current_min_v = std::numeric_limits<float>::infinity();
		while(n != 0 && n != comm.getSize()+1) {
			if(comm.getSize()%n == 0) {
				int p = comm.getSize()/n;
				float diff = std::abs(n/p - width/height);
				if (diff < current_min_v) {
					_n = n;
					_p = p;
					current_min_v = diff;
				}
			}
			if (width > height)
				++n;
			else
				--n;
		}
		int process=0;
		process_map.resize(_n);
		for(auto& item : process_map)
			item.resize(_p);

		this->N_x = width/_n;
		this->N_y = height/_p;
		for(int x = 0; x < _n; x++) {
			for(int y = 0; y < _p; y++) {
				process_map[x][y] = process;
				DiscretePoint origin = {x*N_x, y*N_y};
				DiscretePoint extent = {(x+1)*N_x, (y+1)*N_y};
				if(x == _n-1)
					extent.x = width;
				if(y == _p-1)
					extent.y = height;
				process_grid_dimensions[process] = {origin, extent};
				++process;
			}
		}
	}

	int FastProcessMapping::process(DiscretePoint point) const {
		return process_map[std::min((int) point.x/N_x, _n-1)][std::min((int) point.y/N_y, _p-1)];
	}

	int FastProcessMapping::n() const {
		return _n;
	}

	int FastProcessMapping::p() const {
		return _p;
	}

	GridDimensions FastProcessMapping::gridDimensions(int process) const {
		return process_grid_dimensions[process];
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
		: default_process_mapping(0, 0, fpmas::communication::WORLD), grid_process_mapping(grid_process_mapping) {
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
