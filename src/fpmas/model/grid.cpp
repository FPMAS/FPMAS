#include "grid.h"
#include "fpmas/random/generator.h"

namespace fpmas { namespace model {

	GridCellFactory<> VonNeumannGridBuilder::default_cell_factory;

	void VonNeumannGridBuilder::allocate(
			CellMatrix& matrix,
			DiscreteCoordinate width,
			DiscreteCoordinate height) {
		matrix.resize(height);
		for(auto& row : matrix)
			row.resize(width);
	}
	typename VonNeumannGridBuilder::CellMatrix VonNeumannGridBuilder::buildLocalGrid(
			api::model::SpatialModel& model,
			DiscreteCoordinate min_x, DiscreteCoordinate max_x,
			DiscreteCoordinate min_y, DiscreteCoordinate max_y) {

		DiscreteCoordinate local_height = max_y - min_y + 1;
		DiscreteCoordinate local_width = max_x - min_x + 1;

		CellMatrix cells;
		allocate(cells, local_width, local_height);

		for(DiscreteCoordinate y = min_y; y <= max_y; y++) {
			for(DiscreteCoordinate x = min_x; x <= max_x; x++) {
				auto cell = cell_factory.build({x, y});
				cells[y-min_y][x-min_x] = cell;
				model.add(cell);
			}
		}
		for(DiscreteCoordinate j = 0; j < local_width; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j], api::model::NEIGHBOR_CELL);
			}
			for(DiscreteCoordinate i = local_height-1; i > 0; i--) {
				model.link(cells[i][j], cells[i-1][j], api::model::NEIGHBOR_CELL);
			}
		}
		for(DiscreteCoordinate i = 0; i < local_height; i++) {
			for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
				model.link(cells[i][j], cells[i][j+1], api::model::NEIGHBOR_CELL);
			}
			for(DiscreteCoordinate j = local_width-1; j > 0; j--) {
				model.link(cells[i][j], cells[i][j-1], api::model::NEIGHBOR_CELL);
			}
		}

		return cells;
	}

	void VonNeumannGridBuilder::build(api::model::SpatialModel& model) {
		typedef std::pair<DistributedId, std::vector<api::model::GroupId>>
			GridCellPack;
		if(this->width * this->height > 0) {
			auto& mpi_comm = model.graph().getMpiCommunicator();
			fpmas::communication::TypedMpi<std::vector<GridCellPack>> mpi(mpi_comm);
			if(this->width >= this->height) {
				DiscreteCoordinate column_per_proc = this->width / mpi_comm.getSize();
				DiscreteCoordinate remainder = this->width % mpi_comm.getSize();

				DiscreteCoordinate local_columns_count = column_per_proc;
				FPMAS_ON_PROC(mpi_comm, mpi_comm.getSize()-1)
					local_columns_count+=remainder;

				DiscreteCoordinate begin_width = mpi_comm.getRank() * column_per_proc;
				DiscreteCoordinate end_width = begin_width + local_columns_count - 1;

				buildLocalGrid(model, begin_width, end_width, 0, height-1);
			} else {
				DiscreteCoordinate rows_per_proc = this->height / mpi_comm.getSize();
				DiscreteCoordinate remainder = this->height % mpi_comm.getSize();

				DiscreteCoordinate local_rows_count = rows_per_proc;
				FPMAS_ON_PROC(mpi_comm, mpi_comm.getSize()-1)
					local_rows_count+=remainder;

				DiscreteCoordinate begin_height = mpi_comm.getRank() * rows_per_proc;
				DiscreteCoordinate end_height = begin_height + local_rows_count - 1;

				CellMatrix cells = buildLocalGrid(model, 0, this->width-1, begin_height, end_height);

				std::unordered_map<int, std::vector<GridCellPack>> frontiers;
				if(mpi_comm.getRank() < mpi_comm.getSize() - 1) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate x = 0; x < this->width; x++) {
						auto cell = cells[end_height-begin_height][x];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()+1] = frontier;
				}
				if (mpi_comm.getRank() > 0) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate x = 0; x < this->width; x++) {
						auto cell = cells[0][x];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()-1] = frontier;
				}
				frontiers = mpi.allToAll(frontiers);

				for(auto frontier : frontiers) {
					for(DiscreteCoordinate x = 0; x < this->width; x++) {
						auto cell_pack = frontier.second[x];
						DiscretePoint point;
						if(frontier.first < mpi_comm.getRank()) {
							point = {x, begin_height-1};
						} else {
							point = {x, end_height+1};
						}
						auto cell = cell_factory.build(point);
						for(auto gid : cell_pack.second)
							cell->addGroupId(gid);
						auto tmp_node = new graph::DistributedNode<AgentPtr>(cell_pack.first, cell);
						tmp_node->setLocation(frontier.first);
						model.graph().insertDistant(tmp_node);

						if(frontier.first < mpi_comm.getRank()) {
							model.link(cells[0][x], cell, SpatialModelLayers::NEIGHBOR_CELL);
						} else {
							model.link(cells[end_height-begin_height][x], cell, SpatialModelLayers::NEIGHBOR_CELL);
						}
					}
				}
			}
		}
		model.graph().synchronize();
	}

	struct PointComparison {
		bool operator()(const DiscretePoint& p1, const DiscretePoint& p2) const {
			return p1.x < p2.x && p1.y < p2.y;
		}
	};

	RandomAgentMapping::RandomAgentMapping(
			api::random::Distribution<DiscreteCoordinate>&& x,
			api::random::Distribution<DiscreteCoordinate>&& y,
			std::size_t agent_count,
			std::vector<DiscretePoint> local_points)
	: RandomAgentMapping(x, y, agent_count, local_points) {
	}

	RandomAgentMapping::RandomAgentMapping(
			api::random::Distribution<DiscreteCoordinate>& x,
			api::random::Distribution<DiscreteCoordinate>& y,
			std::size_t agent_count,
			std::vector<DiscretePoint> local_points) {
		std::set<DiscretePoint, PointComparison> local_points_set;
		for(auto point : local_points)
			local_points_set.insert(point);
		random::mt19937_64 rd;

		for(std::size_t i = 0; i < agent_count; i++) {
			DiscretePoint p {x(rd), y(rd)};
			if(local_points_set.count(p) > 0)
				count_map[p.x][p.y]++;
		}
	}
}}

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2) {
		return p1.x == p2.x && p1.y == p2.y;
	}
}}}
