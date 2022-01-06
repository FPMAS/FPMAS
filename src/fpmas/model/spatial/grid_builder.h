#ifndef FPMAS_GRID_BUILDER_H
#define FPMAS_GRID_BUILDER_H

/** \file src/fpmas/model/spatial/grid_builder.h
 * Generic grid builder features.
 */

#include "grid.h"

namespace fpmas { namespace model { namespace detail {


	/**
	 * Utility class used to describe the shape of a discrete grid.
	 *
	 * A grid is described by its origin point and an extent until which the
	 * grid extents.
	 *
	 * The `origin` is included in the grid, while the `extent` is
	 * **not**
	 *
	 * For example, GridDimensions({0, 0}, {10, 10}) describes a grid
	 * where the bottom left corner is at (0, 0), and the top right
	 * corner is at (9, 9).
	 */
	class GridDimensions {
		private:
			DiscretePoint origin;
			DiscretePoint extent;

		public:
			/**
			 * GridDimensions constructor.
			 *
			 * @param origin origin point of the grid
			 * @param extent point to until which the grid extents
			 */
			GridDimensions(DiscretePoint origin, DiscretePoint extent)
				: origin(origin), extent(extent) {
				}
			/**
			 * Default GridDimensions constructor.
			 */
			GridDimensions() {}

			/**
			 * Origin of the grid.
			 */
			DiscretePoint getOrigin() const {return origin;}
			/**
			 * Sets the origin of the grid.
			 */
			void setOrigin(const DiscretePoint& point) {this->origin = point;}

			/**
			 * Extent of the grid.
			 */
			DiscretePoint getExtent() const {return extent;}
			/**
			 * Sets the extent of the grid.
			 */
			void setExtent(const DiscretePoint& point) {this->extent = point;}

			/**
			 * Returns the width of the grid, i.e. the count of cells contained
			 * in the grid width.
			 */
			DiscreteCoordinate width() const {
				return extent.x - origin.x;
			}
			/**
			 * Returns the height of the grid, i.e. the count of cells contained
			 * in the grid height.
			 */
			DiscreteCoordinate height() const {
				return extent.y - origin.y;
			}
	};

	/**
	 * Grid builder base class.
	 *
	 * This class defines a generic algorithm that can be used to build
	 * VonNeumann or Moore grids.
	 *
	 * @see VonNeumannGridBuilder
	 * @see MooreGridBuilder
	 */
	template<typename CellType>
		class GridBuilder : public api::model::CellNetworkBuilder<CellType> {
			protected:
				/**
				 * Type used to describe a cell matrix.
				 *
				 * By convention, the first index represent lines, the second
				 * represents columns.
				 */
				typedef std::vector<std::vector<CellType*>> CellMatrix;

			private:
				api::model::GridCellFactory<CellType>& cell_factory;
				DiscreteCoordinate _width;
				DiscreteCoordinate _height;

				CellMatrix buildLocalCells(
						api::model::SpatialModel<CellType>& model,
						GridDimensions local_dimensions,
						api::model::GroupList groups) const;

				void allocate(
						CellMatrix& matrix,
						DiscreteCoordinate width,
						DiscreteCoordinate height) const;

				std::vector<CellType*> _build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups) const;

			protected:
				/**
				 * Builds links between cells on the current process to build
				 * the required grid shape.
				 *
				 * The specified CellMatrix shape is
				 * [local_dimensions.height()][local_dimensions.width()], and
				 * cells locations are initialized accordingly, i.e. the cell
				 * at `cells[j][i]` is located at DiscretePoint(i, j).
				 * (j = line index = y coordinate, i = column index = x coordinate)
				 *
				 * Links across processes
				 * should **not** be considered in this method.
				 * 
				 * @param model model in which cells are built
				 * @param local_dimensions dimension of the local grid, that is
				 * only a part of the global grid
				 * @param cells built cells to link, corresponding to
				 * local_dimensions
				 */
				virtual void buildLocalGrid(
						api::model::SpatialModel<CellType>& model,
						GridDimensions local_dimensions,
						CellMatrix& cells
						) const = 0;

				/**
				 * Builds links across processes, when the partitioning of the
				 * grid is horizontal, i.e. borders are vertical, i.e. along
				 * the y axis.
				 *
				 * The `frontier` parameter already contains temporary nodes
				 * that are automatically replicated across processes, so that
				 * they can be linked to `local_cells` in order to build the
				 * required grid shape.
				 *
				 * The `frontier` contains two vectors:
				 * - frontier[0]: column of cells replicated from the left
				 *   process, that are expected to be link to `local_cells[y][0]`
				 *   on this process
				 * - frontier[1]: column of cells replicated from the right
				 *   process, that are expected to be link to
				 *   `local_cells[y][local_dimensions.width()-1]`
				 *   on this process
				 *
				 * For the two processes that represent the border of the
				 * global grid, either frontier[0] or frontier[1] is empty.
				 *
				 * `local_cells` and `local_dimensions` are defined as for
				 * buildLocalGrid().
				 *
				 * @param model model in which cells are built
				 * @param local_dimensions dimension of the local grid, that is
				 * only a part of the global grid
				 * @param local_cells built cells to link, corresponding to
				 * local_dimensions
				 * @param frontier temporary DISTANT cells, replicated from
				 * other processes, that should be linked to local cells
				 */
				virtual void linkVerticalFrontiers(
						api::model::SpatialModel<CellType>& model,
						GridDimensions local_dimensions,
						CellMatrix& local_cells,
						const std::array<std::vector<CellType*>, 2>& frontier
						) const = 0;

				/**
				 * Builds links across processes, when the partitioning of the
				 * grid is vertical, i.e. borders are horizontal, i.e. along
				 * the x axis.
				 *
				 * The `frontier` parameter already contains temporary nodes
				 * that are automatically replicated across processes, so that
				 * they can be linked to `local_cells` in order to build the
				 * required grid shape.
				 *
				 * The `frontier` contains two vectors:
				 * - frontier[0]: row of cells replicated from the bottom
				 *   process, that are expected to be link to `local_cells[0][x]`
				 *   on this process
				 * - frontier[1]: row of cells replicated from the top
				 *   process, that are expected to be link to
				 *   `local_cells[local_dimensions.height()-1][x]`
				 *   on this process
				 *
				 * For the two processes that represent the border of the
				 * global grid, either frontier[0] or frontier[1] is empty.
				 *
				 * `local_cells` and `local_dimensions` are defined as for
				 * buildLocalGrid().
				 *
				 * @param model model in which cells are built
				 * @param local_dimensions dimension of the local grid, that is
				 * only a part of the global grid
				 * @param local_cells built cells to link, corresponding to
				 * local_dimensions
				 * @param frontier temporary DISTANT cells, replicated from
				 * other processes, that should be linked to local cells
				 */
				virtual void linkHorizontalFrontiers(
						api::model::SpatialModel<CellType>& model,
						GridDimensions local_dimensions,
						CellMatrix& local_cells,
						const std::array<std::vector<CellType*>, 2>& frontier
						) const = 0;


			public:
				/**
				 * Default GridCellFactory.
				 */
				static GridCellFactory<CellType> default_cell_factory;

				/**
				 * GridBuilder constructor.
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
				GridBuilder(
					api::model::GridCellFactory<CellType>& cell_factory,
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: cell_factory(cell_factory), _width(width), _height(height) {

				}

				/**
				 * GridBuilder constructor.
				 *
				 * The grid will be built using the default_cell_factory.
				 *
				 * @param width width of the grid
				 * @param height height of the grid
				 */
				GridBuilder(
						DiscreteCoordinate width,
						DiscreteCoordinate height)
					: GridBuilder(default_cell_factory, width, height) {

					}

				/**
				 * Grid width.
				 */
				DiscreteCoordinate width() const {
					return _width;
				}
				/**
				 * Grid height.
				 */
				DiscreteCoordinate height() const {
					return _height;
				}

				/**
				 * Builds a grid into the specified `spatial_model`, according
				 * to the grid shape specified by the buildLocalGrid(),
				 * linkHorizontalFrontiers() and linkVerticalFrontiers()
				 * methods.
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
				 * @param spatial_model spatial model in which cells will be added
				 * @return cells built on the current process
				 */
				std::vector<CellType*> build(
						api::model::SpatialModel<CellType>& spatial_model) const override {
					return this->_build(spatial_model, {});
				}

				/**
				 * \copydoc fpmas::api::model::CellNetworkBuilder::build(fpmas::api::model::SpatialModel<CellType>&, fpmas::api::model::GroupList) const
				 */
				std::vector<CellType*> build(
						api::model::SpatialModel<CellType>& spatial_model,
						api::model::GroupList groups) const override {
					return this->_build(spatial_model, groups);
				}


		};

	template<typename CellType>
		GridCellFactory<CellType> GridBuilder<CellType>::default_cell_factory;

	template<typename CellType>
		void GridBuilder<CellType>::allocate(
				CellMatrix& matrix,
				DiscreteCoordinate width,
				DiscreteCoordinate height) const {
			matrix.resize(height);
			for(auto& row : matrix)
				row.resize(width);
		}

	template<typename CellType>
	typename GridBuilder<CellType>::CellMatrix GridBuilder<CellType>::buildLocalCells(
			api::model::SpatialModel<CellType>& model,
			GridDimensions local_dimensions,
			api::model::GroupList groups) const {

		DiscretePoint origin = local_dimensions.getOrigin();
		DiscretePoint extents = local_dimensions.getExtent();

		CellMatrix cells;
		allocate(cells, local_dimensions.width(), local_dimensions.height());

		for(DiscreteCoordinate y = origin.y; y < extents.y; y++) {
			for(DiscreteCoordinate x = origin.x; x < extents.x; x++) {
				auto cell = cell_factory.build({x, y});
				cells[y-origin.y][x-origin.x] = cell;
				model.add(cell);
				for(auto group : groups)
					group.get().add(cell);
			}
		}
		return cells;
	}

	template<typename CellType>
	std::vector<CellType*> GridBuilder<CellType>::_build(
			api::model::SpatialModel<CellType>& model,
			api::model::GroupList groups) const {
		typedef std::pair<DistributedId, std::vector<api::model::GroupId>>
			GridCellPack;

		CellMatrix cells;
		if(this->_width * this->_height > 0) {
			auto& mpi_comm = model.graph().getMpiCommunicator();
			fpmas::communication::TypedMpi<std::vector<GridCellPack>> mpi(mpi_comm);
			GridDimensions local_dimensions;

			if(this->_width >= this->_height) {
				DiscreteCoordinate column_per_proc = this->_width / mpi_comm.getSize();
				DiscreteCoordinate remainder = this->_width % mpi_comm.getSize();

				DiscreteCoordinate local_columns_count = column_per_proc;
				FPMAS_ON_PROC(mpi_comm, mpi_comm.getSize()-1)
					local_columns_count+=remainder;

				DiscreteCoordinate begin_width = mpi_comm.getRank() * column_per_proc;
				DiscreteCoordinate end_width = begin_width + local_columns_count;

				local_dimensions = {
					{begin_width, 0},
					{end_width, this->_height}
				};

				//cells = buildLocalCells(
						//model, begin_width, end_width, 0, this->_height-1, groups
						//);
				cells = buildLocalCells(
						model, local_dimensions, groups
						);
				buildLocalGrid(model, local_dimensions, cells);

				std::unordered_map<int, std::vector<GridCellPack>> mpi_frontiers;
				if(mpi_comm.getRank() < mpi_comm.getSize() - 1) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate y = 0; y < this->_height; y++) {
						auto cell = cells[y][end_width-begin_width-1];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					mpi_frontiers[mpi_comm.getRank()+1] = frontier;
				}
				if (mpi_comm.getRank() > 0) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate y = 0; y < this->_height; y++) {
						auto cell = cells[y][0];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					mpi_frontiers[mpi_comm.getRank()-1] = frontier;
				}
				mpi_frontiers = mpi.allToAll(mpi_frontiers);

				std::array<std::vector<CellType*>, 2> cell_frontiers;
				for(auto frontier : mpi_frontiers) {
					for(DiscreteCoordinate y = 0; y < this->_height; y++) {
						auto cell_pack = frontier.second[y];
						DiscretePoint point;
						if(frontier.first < mpi_comm.getRank()) {
							point = {begin_width-1, y};
						} else {
							point = {end_width, y};
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
							cell_frontiers[0].push_back(
									dynamic_cast<CellType*>(tmp_node->data().get())
									);
						} else {
							// idem
							cell_frontiers[1].push_back(
									dynamic_cast<CellType*>(tmp_node->data().get())
									);
						}
					}
				}
				linkVerticalFrontiers(model, local_dimensions, cells, cell_frontiers);
			} else {
				DiscreteCoordinate rows_per_proc = this->_height / mpi_comm.getSize();
				DiscreteCoordinate remainder = this->_height % mpi_comm.getSize();

				DiscreteCoordinate local_rows_count = rows_per_proc;
				FPMAS_ON_PROC(mpi_comm, mpi_comm.getSize()-1)
					local_rows_count+=remainder;

				DiscreteCoordinate begin_height = mpi_comm.getRank() * rows_per_proc;
				DiscreteCoordinate end_height = begin_height + local_rows_count;

				GridDimensions local_dimensions {
					{0, begin_height},
					{this->_width, end_height}
				};

				cells = buildLocalCells(
						model, local_dimensions, groups
						);
				buildLocalGrid(model, local_dimensions, cells);

				std::unordered_map<int, std::vector<GridCellPack>> frontiers;
				if(mpi_comm.getRank() < mpi_comm.getSize() - 1) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate x = 0; x < this->_width; x++) {
						auto cell = cells[end_height-begin_height-1][x];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()+1] = frontier;
				}
				if (mpi_comm.getRank() > 0) {
					std::vector<GridCellPack> frontier;
					for(DiscreteCoordinate x = 0; x < this->_width; x++) {
						auto cell = cells[0][x];
						frontier.push_back({cell->node()->getId(), cell->groupIds()});
					}
					frontiers[mpi_comm.getRank()-1] = frontier;
				}
				frontiers = mpi.allToAll(frontiers);

				std::array<std::vector<CellType*>, 2> cell_frontiers;
				for(auto frontier : frontiers) {
					for(DiscreteCoordinate x = 0; x < this->_width; x++) {
						auto cell_pack = frontier.second[x];
						DiscretePoint point;
						if(frontier.first < mpi_comm.getRank()) {
							point = {x, begin_height-1};
						} else {
							point = {x, end_height};
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
							cell_frontiers[0].push_back(
									dynamic_cast<CellType*>(tmp_node->data().get())
									);
						} else {
							cell_frontiers[1].push_back(
									dynamic_cast<CellType*>(tmp_node->data().get())
									);
						}
					}
				}
				linkHorizontalFrontiers(model, local_dimensions, cells, cell_frontiers);
			}
		}
		model.graph().synchronize();
		std::vector<CellType*> built_cells;
		for(auto row : cells)
			for(auto cell : row)
				built_cells.push_back(cell);
		return built_cells;
	}

}}}
#endif
