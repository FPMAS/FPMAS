#ifndef FPMAS_GRID_BUILDER_H
#define FPMAS_GRID_BUILDER_H

/** \file src/fpmas/model/spatial/grid_builder.h
 * Generic grid builder features.
 */

#include "grid.h"
#include <fpmas/random/random.h>
#include "grid_load_balancing.h"

namespace fpmas { namespace model { namespace detail {


	/**
	 * An index representing grid cells.
	 */
	typedef random::Index<DiscretePoint> GridCellIndex;

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

				mutable std::map<DiscretePoint, std::size_t> built_cells;
				mutable GridCellIndex cell_begin {&built_cells};
				mutable GridCellIndex cell_end {&built_cells};
				mutable std::map<GridCellIndex, CellType*> cell_index;

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

				virtual void linkFrontiers(
						api::model::SpatialModel<CellType>& model,
						GridDimensions local_dimensions,
						CellMatrix& local_cells,
						std::vector<CellType*>& frontier
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
				/**
				 * Initializes a sample of `n` cells selected from the
				 * previously built cells with the provided `init_function`.
				 *
				 * The sample of cells selected is **deterministic**: it is
				 * guaranteed that the same cells are initialized independently
				 * of the current distribution.
				 *
				 * The selection process can be seeded with fpmas::seed().
				 *
				 * Successive calls can be used to independently initialize
				 * several cell states.
				 *
				 * @par Example
				 * ```cpp
				 * fpmas::model::MooreGridBuilder<UserCell> grid_builder(width, height);
				 * grid_builder.build(
				 * 	grid_model,
				 * 	{cell_group}
				 * );
				 * // Sets 10 random cells to be GROWN
				 * grid_builder.initSample(
				 * 	10, [] (UserCell* cell) {
				 * 		cell->setState(GROWN);
				 * 	}
				 * );
				 * // Sets 15 random agents to be RED
				 * grid_builder.initSample(
				 * 	15, [] (UserCell* cell) {
				 * 		cell->setColor(RED);
				 * 	}
				 * );
				 * ```
				 *
				 * @param n sample size
				 * @param init_function initialization function
				 */
				void initSample(
						std::size_t n,
						std::function<void(CellType*)> init_function
						) const;

				/**
				 * Sequentially initializes built cells from the input `items`.
				 *
				 * Each item is assigned to a cell using the specified
				 * `init_function`.
				 *
				 * The item assignment is **deterministic**: it is guaranteed
				 * that each cell is always initialized with the same item
				 * independently of the current distribution.
				 *
				 ** @par Example
				 * ```cpp
				 * fpmas::model::MooreGridBuilder<UserCell> grid_builder(width, height);
				 *
				 * grid_builder.build(
				 * 	grid_model,
				 * 	{cell_group}
				 * );
				 *
				 * // Vector containing an item for each cell
				 * std::vector<unsigned long> items(width * height);
				 *
				 * // items initialization
				 * ...
				 *
				 * // Assign items to agents
				 * grid_builder.initSequence(
				 * 	items, [] (
				 * 		UserCell* cell,
				 * 		const unsigned long& item
				 * 		) {
				 * 		cell->setData(item);
				 * 	}
				 * );
				 * ```
				 *
				 * @param items Items to assign to built cells. The number of
				 * items must be greater or equal to the total number of built
				 * cells, `N=width()*height()`. Only the first `N` items are
				 * assigned, other are ignored.
				 * @param init_function item assignment function
				 */
				template<typename T>
					void initSequence(
							const std::vector<T>& items,
							std::function<void(
								CellType*,
								typename std::vector<T>::const_reference
								)> init_function
							) const;

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

		for(DiscreteCoordinate y = 0; y < this->height(); y++)
			for(DiscreteCoordinate x = 0; x < this->width(); x++)
				built_cells.insert(std::pair<api::model::DiscretePoint, std::size_t>(
							{x, y}, 1ul
							));
		
		cell_begin = GridCellIndex::begin(&built_cells);
		cell_end = GridCellIndex::end(&built_cells);

		for(auto row : cells) {
			for(auto cell : row) {
				// GridCellIndex offset is always 0 in this case (at most one
				// cell per coordinate)
				GridCellIndex index(&built_cells, cell->location(), 0);
				cell_index.insert({index, cell});
			}
		}

		// Random cell seed initialization
		std::vector<std::mt19937_64::result_type> local_seeds(
					GridCellIndex::distance(cell_begin, cell_end)
					);

		for(std::size_t i = 0; i < local_seeds.size(); i++)
			// Generates a unique integer for each, independently from the
			// current distribution
			local_seeds[i] = i;
		// Genererates a better seed distribution. The same seeds are
		// generated on all processes.
		std::seed_seq seed_generator(local_seeds.begin(), local_seeds.end());
		seed_generator.generate(local_seeds.begin(), local_seeds.end());

		initSequence(local_seeds, [] (
					CellType* cell,
					const std::mt19937_64::result_type& seed) {
				cell->seed(seed);
				});

		return cells;
	}

	template<typename CellType>
	std::vector<CellType*> GridBuilder<CellType>::_build(
			api::model::SpatialModel<CellType>& model,
			api::model::GroupList groups) const {
		typedef std::pair<std::pair<DistributedId, DiscretePoint>, std::vector<api::model::GroupId>>
			GridCellPack;

		CellMatrix cells;
		auto& mpi_comm = model.graph().getMpiCommunicator();
		fpmas::communication::TypedMpi<std::vector<GridCellPack>> mpi(mpi_comm);

		TreeProcessMapping tree_process_mapping(
				this->width(), this->height(), mpi_comm
				);
		// Represents the local grid shape built on this process
		GridDimensions local_dimensions
			= tree_process_mapping.gridDimensions(mpi_comm.getRank());

		// Build cells
		cells = buildLocalCells(
				model, local_dimensions, groups
				);
		// Build local links
		buildLocalGrid(model, local_dimensions, cells);

		std::unordered_map<int, std::vector<GridCellPack>> mpi_frontiers;

		/* exchange frontiers */
		/*
		 * Exchange process:
		 * When the TreeProcessMapping is used, the intuitive distribution on 4
		 * processes looks as follows:
		 * +------+------+
		 * |  1   |  2   |
		 * +------+------+
		 * |  0   |  3   |
		 * +------+------+
		 *
		 * In this case, the right frontier of 0 is sent to 3, the top frontier
		 * of 0 is sent to 1, and the top right corner of 0 is sent to 2.
		 *
		 * There are however specific cases about corners, see below.
		 */
		 
		DiscreteCoordinate left_x = local_dimensions.getOrigin().x-1;
		if(left_x >= 0) {
			// left frontier
			for(DiscreteCoordinate y = 1; y < local_dimensions.height()-1; y++) {
				auto cell = cells[y][0];
				std::set<int> processes = {
					tree_process_mapping.process({left_x, cell->location().y})
				};
				if(cell->location().y+1 < this->height())
					processes.insert(
							tree_process_mapping.process({left_x, cell->location().y+1})
							);
				if(cell->location().y-1 >= 0)
					processes.insert(
							tree_process_mapping.process({left_x, cell->location().y-1})
							);

				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}
		}
		DiscreteCoordinate bottom_y = local_dimensions.getOrigin().y-1;
		if(bottom_y >= 0) {
			// bottom frontier
			for(DiscreteCoordinate x = 1; x < local_dimensions.width()-1; x++) {
				auto cell = cells[0][x];
				std::set<int> processes = {
					tree_process_mapping.process({cell->location().x, bottom_y})
				};
				if(cell->location().x+1 < this->width())
					processes.insert(
							tree_process_mapping.process({cell->location().x+1, bottom_y})
					);
				if(cell->location().x-1 >= 0)
					processes.insert(
							tree_process_mapping.process({cell->location().x-1, bottom_y})
					);
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}

		}
		DiscreteCoordinate right_x = local_dimensions.getExtent().x;
		if(right_x < this->width()) {
			// right frontier
			for(DiscreteCoordinate y = 1; y < local_dimensions.height()-1; y++) {
				auto cell = cells[y][local_dimensions.width()-1];
				std::set<int> processes = {
					tree_process_mapping.process({right_x, cell->location().y})
				};
				if(cell->location().y+1 < this->height())
					processes.insert(
							tree_process_mapping.process({right_x, cell->location().y+1})
					);
				// if y == 0, this corner is checked at bottom frontier
				if(cell->location().y-1 >= 0)
					processes.insert(
							tree_process_mapping.process({right_x, cell->location().y-1})
					);
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}
		}
		
		DiscreteCoordinate top_y = local_dimensions.getExtent().y;
		if(top_y < this->height()) {
			// top frontier
			for(DiscreteCoordinate x = 1; x < local_dimensions.width()-1; x++) {
				auto cell = cells[local_dimensions.height()-1][x];
				std::set<int> processes = {
					tree_process_mapping.process({cell->location().x, top_y})
				};
				
				// if x == local_dimensions.width()-1, this corner is checked
				// by the right frontier
				if(cell->location().x+1 < this->width())
					processes.insert(
							tree_process_mapping.process({cell->location().x+1, top_y})
							);
				if(cell->location().x-1 >= 0)
					processes.insert(
							tree_process_mapping.process({cell->location().x-1, top_y})
							);

				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}
		}

		/* corners */

		/*
		 * Note about corners:
		 *
		 * When the TreeProcessMapping is used, the distributed grid might have
		 * the following shape on 4 processes (where 3's width is greater than
		 * 2's width):
		 * +------+---+----+
		 * |  1   |   |    |
		 * +------| 2 |  3 |
		 * |  0   |   |    |
		 * +------+---+----+
		 *
		 * In that case, for example, the top right corner of the process 0
		 * should **not** be sent again to 2, since it has already been sent as
		 * the right frontier of process 0.
		 *
		 * The following cases prevent all those issues.
		 */
		if(local_dimensions.height() > 0 && local_dimensions.width() > 0) {
			// Bottom left corner
			{
				auto cell = cells[0][0];
				std::set<int> processes;
				auto x = cell->location().x;
				auto y = cell->location().y;
				if(y-1 > 0) {
					processes.insert(
							tree_process_mapping.process({x, y-1})
							);
					if(x-1 > 0)
						processes.insert(
								tree_process_mapping.process({x-1, y-1})
								);
					if(x+1 < this->width()-1)
						processes.insert(
								tree_process_mapping.process({x+1, y-1})
								);
				}
				if(x-1 > 0) {
					processes.insert(
							tree_process_mapping.process({x-1, y})
							);
					if(y+1 < this->height()-1)
						processes.insert(
								tree_process_mapping.process({x-1, y+1})
								);
				}
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}

			{
				// Bottom right corner
				auto cell = cells[0][local_dimensions.width()-1];
				std::set<int> processes;
				auto x = cell->location().x;
				auto y = cell->location().y;
				if(y-1 > 0) {
					processes.insert(
							tree_process_mapping.process({x, y-1})
							);
					if(x-1 > 0)
						processes.insert(
								tree_process_mapping.process({x-1, y-1})
								);
					if(x+1 < this->width())
						processes.insert(
								tree_process_mapping.process({x+1, y-1})
								);
				}
				if(x+1 < this->width()) {
					processes.insert(
							tree_process_mapping.process({x+1, y})
							);
					if(y+1 < this->height()-1)
						processes.insert(
								tree_process_mapping.process({x+1, y+1})
								);
				}
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}

			// Top left corner
			{
				auto cell = cells[local_dimensions.height()-1][0];
				std::set<int> processes;
				auto x = cell->location().x;
				auto y = cell->location().y;
				if(y+1 < this->height()) {
					processes.insert(
							tree_process_mapping.process({x, y+1})
							);
					if(x-1 > 0)
						processes.insert(
								tree_process_mapping.process({x-1, y+1})
								);
					if(x+1 < this->width()-1)
						processes.insert(
								tree_process_mapping.process({x+1, y+1})
								);
				}
				if(x-1 > 0) {
					processes.insert(
							tree_process_mapping.process({x-1, y})
							);
					if(y-1 > 0)
						processes.insert(
								tree_process_mapping.process({x-1, y+1})
								);
				}
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}

			// Top right corner
			{
				auto cell = cells[local_dimensions.height()-1][local_dimensions.width()-1];
				std::set<int> processes;
				auto x = cell->location().x;
				auto y = cell->location().y;
				if(y+1 < this->height()) {
					processes.insert(
							tree_process_mapping.process({x, y+1})
							);
					if(x-1 > 0)
						processes.insert(
								tree_process_mapping.process({x-1, y+1})
								);
					if(x+1 < this->width())
						processes.insert(
								tree_process_mapping.process({x+1, y+1})
								);
				}
				if(x+1 < this->width()) {
					processes.insert(
							tree_process_mapping.process({x+1, y})
							);
					if(y-1 > 0)
						processes.insert(
								tree_process_mapping.process({x+1, y-1})
								);
				}
				for(int process : processes) {
					mpi_frontiers[process].push_back(
							{{cell->node()->getId(), cell->location()}, cell->groupIds()});
				}
			}
		}

		

		mpi_frontiers = mpi.allToAll(mpi_frontiers);

		std::vector<CellType*> cell_frontiers;
		for(auto frontier : mpi_frontiers) {
			for(auto cell_pack : frontier.second) {
				// Builds a temporary cell
				auto cell = cell_factory.build(cell_pack.first.second);
				// Cells group ids must be updated before it is
				// inserted in the graph
				for(auto gid : cell_pack.second)
					cell->addGroupId(gid);

				// Builds a temporary node representing the node on the
				// frontier (that is located on an other process)
				api::graph::DistributedNode<AgentPtr>* tmp_node
					= new graph::DistributedNode<AgentPtr>(cell_pack.first.first, cell);
				tmp_node->setLocation(frontier.first);
				model.graph().insertDistant(tmp_node);
				cell_frontiers.push_back(cell);
			}
		}
		linkFrontiers(model, local_dimensions, cells, cell_frontiers);

		model.graph().synchronize();
		std::vector<CellType*> built_cells;
		for(auto row : cells)
			for(auto cell : row)
				built_cells.push_back(cell);
		return built_cells;
	}

	template<typename CellType>
		void GridBuilder<CellType>::initSample(
				std::size_t n,
				std::function<void(CellType*)> init_function) const {
			std::vector<GridCellIndex> indexes = random::sample_indexes(
					cell_begin, cell_end, n, RandomGridAgentBuilder::rd);
			for(auto index : indexes) {
				auto it = cell_index.find(index);
				if(it != cell_index.end())
					init_function(it->second);
			}
		}

	template<typename CellType>
		template<typename T>
		void GridBuilder<CellType>::initSequence(
				const std::vector<T> &items,
				std::function<void (
					CellType*,
					typename std::vector<T>::const_reference
					)> init_function) const {
			for(auto cell : cell_index)
				init_function(
						cell.second,
						items[GridCellIndex::distance(cell_begin, cell.first)]
						);
		}

}}}
#endif
