#ifndef FPMAS_VON_NEUMANN_H
#define FPMAS_VON_NEUMANN_H

#include "grid.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	class ManhattanDistance {
		public:
			DiscreteCoordinate operator()(
					const DiscretePoint& p1,
					const DiscretePoint& p2) const {
				return std::abs(p2.x - p1.x) + std::abs(p2.y - p1.y);
			}
	};

	class VonNeumannGridBuilder : public api::model::SpatialModelBuilder<api::model::GridCell> {
		private:
			typedef std::vector<std::vector<api::model::GridCell*>> CellMatrix;
			void allocate(CellMatrix& matrix, DiscreteCoordinate width, DiscreteCoordinate height);

			api::model::GridCellFactory& cell_factory;
			DiscreteCoordinate width;
			DiscreteCoordinate height;

			CellMatrix buildLocalGrid(
					api::model::GridModel& model,
					DiscreteCoordinate min_x, DiscreteCoordinate max_x,
					DiscreteCoordinate min_y, DiscreteCoordinate max_y);
			CellMatrix buildLocalGrid(
					DiscreteCoordinate min_x, DiscreteCoordinate max_x,
					DiscreteCoordinate min_y, DiscreteCoordinate max_y);

		public:
			static GridCellFactory<> default_cell_factory;

			VonNeumannGridBuilder(
					api::model::GridCellFactory& cell_factory,
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: cell_factory(cell_factory), width(width), height(height) {}
			VonNeumannGridBuilder(
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: VonNeumannGridBuilder(default_cell_factory, width, height) {}

			api::model::GridCellFactory& gridCellFactory() {
				return cell_factory;
			}
			std::vector<api::model::GridCell*> build(
					api::model::GridModel& spatial_model);
	};

	typedef GridConfig<VonNeumannGridBuilder, ManhattanDistance> VonNeumannGrid;

	/**
	 * VonNeumannRange perimeter function object.
	 *
	 * **Must** be explicitly specialized for each available GridConfig.
	 */
	template<typename GridConfig>
		struct VonNeumannRangePerimeter {};
	template<typename GridConfig>
	using VonNeumannRangeConfig = GridRangeConfig<ManhattanDistance, VonNeumannRangePerimeter<GridConfig>>;

	template<>
		struct VonNeumannRangePerimeter<VonNeumannGrid> {
			DiscretePoint operator()(const GridRange<VonNeumannGrid, VonNeumannRangeConfig<VonNeumannGrid>>& range) const {
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
	
	template<typename GridConfig>
		using VonNeumannRange = GridRange<GridConfig, VonNeumannRangeConfig<GridConfig>>;

}}
#endif
