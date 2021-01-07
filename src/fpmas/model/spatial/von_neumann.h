#ifndef FPMAS_VON_NEUMANN_H
#define FPMAS_VON_NEUMANN_H

/** \file src/fpmas/model/spatial/von_neumann.h
 * VonNeumann neighborhood related objects.
 */

#include "grid.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * Function object used to compute the [Manhattan
	 * distance](https://en.wikipedia.org/wiki/Taxicab_geometry) between two
	 * DiscretePoints.
	 */
	class ManhattanDistance {
		public:
			/**
			 * Computes the Manhattan distance between `p1` and `p2`.
			 *
			 * @return Manhattan distance between `p1` and `p2`
			 */
			DiscreteCoordinate operator()(
					const DiscretePoint& p1,
					const DiscretePoint& p2) const {
				return std::abs(p2.x - p1.x) + std::abs(p2.y - p1.y);
			}
	};

	/**
	 * CellNetworkBuilder implementation used to build a VonNeumann grid.
	 *
	 * The built grid is such the successors of each \GridCell correspond to
	 * \GridCells of its [VonNeumann
	 * neighborhood](https://en.wikipedia.org/wiki/Von_Neumann_neighborhood).
	 *
	 * Notice that this only has an impact on internal performances, notably
	 * concerning the DistributedMoveAlgorithm: \GridAgents' mobility and
	 * perception ranges are **not** limited to the "Von Neumann shape" of the
	 * Grid.
	 *
	 * @see VonNeumannRange
	 * @see MooreRange
	 */
	template<typename CellType = api::model::GridCell>
	class VonNeumannGridBuilder : public api::model::CellNetworkBuilder<CellType> {
		private:
			typedef std::vector<std::vector<CellType*>> CellMatrix;
			void allocate(CellMatrix& matrix, DiscreteCoordinate width, DiscreteCoordinate height) const;

			api::model::GridCellFactory<CellType>& cell_factory;
			DiscreteCoordinate width;
			DiscreteCoordinate height;

			CellMatrix buildLocalGrid(
					api::model::SpatialModel<CellType>& model,
					DiscreteCoordinate min_x, DiscreteCoordinate max_x,
					DiscreteCoordinate min_y, DiscreteCoordinate max_y) const;

		public:
			/**
			 * Default GridCellFactory.
			 */
			static GridCellFactory<CellType> default_cell_factory;

			/**
			 * VonNeumannGridBuilder constructor.
			 *
			 * The `cell_factory` parameter allows to build the grid using
			 * custom GridCell extensions (with extra user defined behaviors for
			 * example).
			 *
			 * @param cell_factory custom cell factory, that will be used
			 * instead of default_cell_factory
			 * @param width width of the grid
			 * @param height height of the grid
			 */
			VonNeumannGridBuilder(
					api::model::GridCellFactory<CellType>& cell_factory,
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: cell_factory(cell_factory), width(width), height(height) {}
			/**
			 * VonNeumannGridBuilder constructor.
			 *
			 * The grid will be built using the default_cell_factory.
			 *
			 * @param width width of the grid
			 * @param height height of the grid
			 */
			VonNeumannGridBuilder(
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: VonNeumannGridBuilder(default_cell_factory, width, height) {}

			/**
			 * Builds a VonNeumann grid into the specified `spatial_model`.
			 *
			 * The process is distributed so that each available process
			 * instantiates a part of the global grid.
			 *
			 * The size of the global grid is equal to `width x height`,
			 * according to the parameters specified in the constructor. The
			 * origin of the grid is considered at (0, 0), and so the opposite
			 * corner is at (width-1, height-1).
			 *
			 * Each GridCell is built using the `cell_factory` specified in the
			 * constructor, the `default_cell_factory` otherwise.
			 *
			 * The successors of each GridCell correspond to \GridCells
			 * contained in its VonNeumann neighborhood (excluding the GridCell
			 * itself).
			 *
			 * @param spatial_model spatial model in which cells will be added
			 * @return cells built on the current process
			 */
			std::vector<CellType*> build(
					api::model::SpatialModel<CellType>& spatial_model) const override;
	};
	template<typename CellType>
	GridCellFactory<CellType> VonNeumannGridBuilder<CellType>::default_cell_factory;

	template<typename CellType>
	void VonNeumannGridBuilder<CellType>::allocate(
			CellMatrix& matrix,
			DiscreteCoordinate width,
			DiscreteCoordinate height) const {
		matrix.resize(height);
		for(auto& row : matrix)
			row.resize(width);
	}

	template<typename CellType>
	typename VonNeumannGridBuilder<CellType>::CellMatrix VonNeumannGridBuilder<CellType>::buildLocalGrid(
			api::model::SpatialModel<CellType>& model,
			DiscreteCoordinate min_x, DiscreteCoordinate max_x,
			DiscreteCoordinate min_y, DiscreteCoordinate max_y) const {

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
				model.link(cells[i][j], cells[i+1][j], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate i = local_height-1; i > 0; i--) {
				model.link(cells[i][j], cells[i-1][j], api::model::CELL_SUCCESSOR);
			}
		}
		for(DiscreteCoordinate i = 0; i < local_height; i++) {
			for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
				model.link(cells[i][j], cells[i][j+1], api::model::CELL_SUCCESSOR);
			}
			for(DiscreteCoordinate j = local_width-1; j > 0; j--) {
				model.link(cells[i][j], cells[i][j-1], api::model::CELL_SUCCESSOR);
			}
		}

		return cells;
	}


	template<typename CellType>
	std::vector<CellType*> VonNeumannGridBuilder<CellType>::build(
			api::model::SpatialModel<CellType>& model) const {
		typedef std::pair<DistributedId, std::vector<api::model::GroupId>>
			GridCellPack;

		CellMatrix cells;
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

				cells = buildLocalGrid(model, begin_width, end_width, 0, height-1);

				std::unordered_map<int, std::vector<GridCellPack>> frontiers;
				if(mpi_comm.getRank() < mpi_comm.getSize() - 1) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate y = 0; y < this->height; y++) {
						auto cell = cells[y][end_width-begin_width];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()+1] = frontier;
				}
				if (mpi_comm.getRank() > 0) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate y = 0; y < this->height; y++) {
						auto cell = cells[y][0];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()-1] = frontier;
				}
				frontiers = mpi.allToAll(frontiers);

				for(auto frontier : frontiers) {
					for(DiscreteCoordinate y = 0; y < this->height; y++) {
						auto cell_pack = frontier.second[y];
						DiscretePoint point;
						if(frontier.first < mpi_comm.getRank()) {
							point = {begin_width-1, y};
						} else {
							point = {end_width+1, y};
						}
						// Builds a temporary cell
						auto cell = cell_factory.build(point);
						// Cells group ids must be updated before it is
						// inserted in the graph
						for(auto gid : cell_pack.second)
							cell->addGroupId(gid);

						// Builds a temporary node representing the node on the
						// frontier (that is located on an other process)
						api::graph::DistributedNode<AgentPtr>* tmp_node
							= new graph::DistributedNode<AgentPtr>(cell_pack.first, cell);
						tmp_node->setLocation(frontier.first);

						// The node might already exist (for example, if a
						// previous link operation from an other process as
						// already occured). In this case, tmp_node is deleted
						// and the existing node is returned, so finally
						// tmp_node is up to date.
						tmp_node = model.graph().insertDistant(tmp_node);

						if(frontier.first < mpi_comm.getRank()) {
							// tmp_node->data() is always up to date, cf above
							model.link(cells[y][0], tmp_node->data(), SpatialModelLayers::CELL_SUCCESSOR);
						} else {
							// idem
							model.link(cells[y][end_width-begin_width], tmp_node->data(), SpatialModelLayers::CELL_SUCCESSOR);
						}
					}
				}
			} else {
				DiscreteCoordinate rows_per_proc = this->height / mpi_comm.getSize();
				DiscreteCoordinate remainder = this->height % mpi_comm.getSize();

				DiscreteCoordinate local_rows_count = rows_per_proc;
				FPMAS_ON_PROC(mpi_comm, mpi_comm.getSize()-1)
					local_rows_count+=remainder;

				DiscreteCoordinate begin_height = mpi_comm.getRank() * rows_per_proc;
				DiscreteCoordinate end_height = begin_height + local_rows_count - 1;

				cells = buildLocalGrid(model, 0, this->width-1, begin_height, end_height);

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
						// Same procedure as above
						auto cell = cell_factory.build(point);
						for(auto gid : cell_pack.second)
							cell->addGroupId(gid);
						api::graph::DistributedNode<AgentPtr>* tmp_node
							= new graph::DistributedNode<AgentPtr>(cell_pack.first, cell);
						tmp_node->setLocation(frontier.first);
						tmp_node = model.graph().insertDistant(tmp_node);

						if(frontier.first < mpi_comm.getRank()) {
							model.link(cells[0][x], tmp_node->data(), SpatialModelLayers::CELL_SUCCESSOR);
						} else {
							model.link(cells[end_height-begin_height][x], tmp_node->data(), SpatialModelLayers::CELL_SUCCESSOR);
						}
					}
				}
			}
		}
		model.graph().synchronize();
		std::vector<CellType*> built_cells;
		for(auto row : cells)
			for(auto cell : row)
				built_cells.push_back(cell);
		return built_cells;
	}

	/**
	 * VonNeumann GridConfig specialization, that might be used where a
	 * `GridConfig` template parameter is required.
	 */
	template<typename CellType = api::model::GridCell>
	using VonNeumannGrid = GridConfig<VonNeumannGridBuilder<CellType>, ManhattanDistance, CellType>;

	/**
	 * VonNeumannRange perimeter function object.
	 *
	 * **Must** be explicitly specialized for each available GridConfig.
	 *
	 * See GridRangeConfig::Perimeter for more information about what a
	 * "perimeter" is.
	 */
	template<typename GridConfig>
		struct VonNeumannRangePerimeter {};

	/**
	 * VonNeumann GridRangeConfig specialization, that might be used where a
	 * `GridRangeConfig` template parameter is required.
	 *
	 * The explicit specialization of VonNeumannRangePerimeter corresponding to
	 * `GridConfig`, i.e. `VonNeumannRangePerimeter<GridConfig>` is
	 * automatically selected: the type is ill-formed if no such specialization
	 * exists.
	 */
	template<typename GridConfig>
	using VonNeumannRangeConfig = GridRangeConfig<ManhattanDistance, VonNeumannRangePerimeter<GridConfig>>;

	/**
	 * VonNeumannRangePerimeter specialization corresponding to a
	 * VonNeumannRange used on a VonNeumannGrid.
	 */
	template<typename CellType>
		struct VonNeumannRangePerimeter<VonNeumannGrid<CellType>> {
			/**
			 * Returns the _perimeter_ of the specified VonNeumann `range` on a
			 * VonNeumannGrid.
			 *
			 * In this particular case, the perimeter corresponds to `(0,
			 * range.getSize())`. See GridRangeConfig::Perimeter for more
			 * information about the definition of a "perimeter".
			 *
			 * @param range VonNeumann range for which the perimeter must be
			 * computed
			 */
			DiscretePoint operator()(const GridRange<VonNeumannGrid<CellType>, VonNeumannRangeConfig<VonNeumannGrid<CellType>>>& range) const {
				return {0, range.getSize()};
			}
		};

	// MooreGrid case: not available yet
	/*
	 *template<>
	 *    struct VonNeumannRangeConfig::Perimeter<MooreGrid> {
	 *        DiscretePoint operator()(const DiscreteCoordinate& range_size) const {
	 *            return {0, range_size};
	 *        }
	 *    };
	 */
	
	/**
	 * GridRange specialization defining variable size ranges with a VonNeumann
	 * shape.
	 *
	 * Formally, considering a VonNeumannRange `range` centered on `p1`, the range is
	 * constituted by any point of the Grid `p` such that `ManhattanDistance()(p1, p) <=
	 * range.getSize()`.
	 *
	 * Notice that this is completely independent from the underlying shape of
	 * the grid, defined by `GridConfig`.
	 *
	 * @see ManhattanDistance
	 * @see MooreRange
	 * @see GridConfig
	 */
	template<typename GridConfig>
		using VonNeumannRange = GridRange<GridConfig, VonNeumannRangeConfig<GridConfig>>;

}}
#endif
