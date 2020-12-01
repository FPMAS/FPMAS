#ifndef FPMAS_GRID_H
#define FPMAS_GRID_H

#include "fpmas/api/model/spatial/grid.h"
#include "spatial_model.h"
#include "fpmas/random/distribution.h"

namespace nlohmann {
	template<>
		struct adl_serializer<fpmas::api::model::DiscretePoint> {
			typedef fpmas::api::model::DiscretePoint DiscretePoint;
			static void to_json(nlohmann::json& j, const DiscretePoint& point) {
				j = {point.x, point.y};
			}
			static void from_json(const nlohmann::json& j, DiscretePoint& point) {
				point.x = j.at(0).get<fpmas::api::model::DiscreteCoordinate>();
				point.y = j.at(1).get<fpmas::api::model::DiscreteCoordinate>();
			}
		};
}

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * DiscretePoint hash function, inspired from `boost::hash_combine`.
	 */
	struct PointHash {
		std::size_t operator()(const DiscretePoint& p) const;
	};
		

	typedef SpatialAgentBuilder<api::model::GridCell> GridAgentBuilder;

	template<typename GridCellType, typename Derived = GridCellType>
	class GridCellBase :
		public api::model::GridCell,
		public Cell<GridCellType, GridCellBase<GridCellType, Derived>> {
		friend nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<GridCellBase<GridCellType, Derived>>>;

		private:
			DiscretePoint _location;
		public:
			typedef GridCellBase<GridCellType, Derived> JsonBase;
			GridCellBase() {}

			GridCellBase(DiscretePoint location)
				: _location(location) {}

			DiscretePoint location() const override {
				return _location;
			}
	};

	class GridCell : public GridCellBase<GridCell> {
		public:
			GridCell() {}
			GridCell(DiscretePoint location)
				: GridCellBase<GridCell>(location) {}

	};

	template<typename AgentType, typename Derived = AgentType>
	class GridAgent :
		public api::model::GridAgent,
		public SpatialAgentBase<AgentType, api::model::GridCell, GridAgent<AgentType, Derived>> {
			friend nlohmann::adl_serializer<api::utils::PtrWrapper<GridAgent<AgentType>>>;

			private:
			DiscretePoint current_location_point;

			protected:
			GridAgent(
					const fpmas::api::model::Range<api::model::GridCell>& mobility_range,
					const fpmas::api::model::Range<api::model::GridCell>& perception_range)
				: model::SpatialAgentBase<AgentType, api::model::GridCell, GridAgent<AgentType, Derived>>(
						mobility_range, perception_range) {}

			using model::SpatialAgentBase<AgentType, api::model::GridCell, GridAgent<AgentType, Derived>>::moveTo;
			void moveTo(api::model::GridCell* cell) override;
			void moveTo(DiscretePoint point) override;

			public:
			DiscretePoint locationPoint() const override {return current_location_point;}
		};

	template<typename AgentType, typename Derived>
		void GridAgent<AgentType, Derived>::moveTo(api::model::GridCell* cell) {
			this->updateLocation(cell);
			current_location_point = cell->location();
		}

	template<typename AgentType, typename Derived>
		void GridAgent<AgentType, Derived>::moveTo(DiscretePoint point) {
			bool found = false;
			auto mobility_field = this->template outNeighbors<fpmas::api::model::GridCell>(SpatialModelLayers::MOVE);
			auto it = mobility_field.begin();
			while(!found && it != mobility_field.end()) {
				if((*it)->location() == point) {
					found=true;
				} else {
					it++;
				}
			}
			if(found)
				this->moveTo(*it);
			else
				throw api::model::OutOfMobilityFieldException(this->node()->getId(), this->locationPoint(), point);
		}

	template<typename GridCellType = GridCell>
		class GridCellFactory : public api::model::GridCellFactory {
			public:
				GridCellType* build(DiscretePoint location) override {
					return new GridCellType(location);
				}
		};

	/**
	 * Generic static Grid configuration interface.
	 */
	template<typename BuilderType, typename DistanceType>
		struct GridConfig {
			/**
			 * fpmas::api::model::SpatialModelBuilder implementation that can
			 * be used to build the grid.
			 */
			typedef BuilderType Builder;
			/**
			 * Function object that defines a function with the following
			 * signature:
			 * ```cpp
			 * \* integer type *\ operator()(const DiscretePoint& p1, const DiscretePoint& p2) const;
			 * ```
			 *
			 * In practice, `p1` and `p2` corresponds to coordinates of Cells
			 * in the grid. The method must returned the length of the **path**
			 * from `p1` to `p2` within the `Graph` representing this `Grid`
			 * (that might have been built using the Builder).
			 *
			 * Here is two examples:
			 * - if Grid cells are linked together using Von Neumann
			 *   neighborhoods, the length of the path from `p1` to `p2` in the
			 *   underlying `Graph` is the ManhattanDistance from `p1` to `p2`.
			 * - if Grid cells are linked together using Moore 
			 *   neighborhoods, the length of the path from `p1` to `p2` in the
			 *   underlying `Graph` is the ChebyshevDistance from `p1` to `p2`.
			 */
			typedef DistanceType Distance;
		};

	/**
	 * Generic static Grid range configuration interface.
	 */
	template<typename DistanceType, typename PerimeterType>
		struct GridRangeConfig {
			/**
			 * Function object used to check if a DiscretePoint is contained
			 * into this range, depending on the considered origin.
			 *
			 * More precisely, the object must provide a method with the
			 * following signature:
			 * ```cpp
			 * DiscreteCoordinate operator()(const DiscretePoint& origin, const DiscretePoint& point_to_check);
			 * ```
			 *
			 * This object is notably used by the GridRange::contains() method to compute
			 * ranges.
			 *
			 * Two classical types of neighborhoods notably use the following
			 * distances:
			 * - [VonNeumann
			 *   neighborhood](https://en.wikipedia.org/wiki/Von_Neumann_neighborhood):
			 *   ManhattanDistance
			 * - [Moore
			 *   neighborhood](https://en.wikipedia.org/wiki/Moore_neighborhood):
			 *   ChebyshevDistance
			 *
			 * See VonNeumannRange and MooreRange for more details.
			 */
			typedef DistanceType Distance;

			/**
			 * Function object that, considering the GridConfig, returns the point
			 * corresponding to the maximum radius of this Range shape, considering
			 * the shape is centered on (0, 0).
			 *
			 * The final radius can be computed as follow:
			 * ```cpp
			 * class Example : public api::model::Range {
			 *     std::size_t radius(api::model::GridCell*) const override {
			 *         typename GridConfig::Distance grid_distance;
			 *         typename RangeType::Perimeter perimeter;
			 *         return grid_distance({0, 0}, perimeter(*this));
			 *     }
			 * };
			 * ```
			 *
			 * This default template does not implement any perimeter, since such a
			 * property highly depends on specified `GridConfig`. Instead, 
			 */
			typedef PerimeterType Perimeter;
		};

	/**
	 * A generic api::model::Range implementation for Grid environments.
	 *
	 * The corresponding ranges are uniform ranges constructed using a `size`
	 * threshold and the `RangeConfig::Distance` function object.
	 */
	template<typename _GridConfig, typename _RangeConfig>
		class GridRange :
			public api::model::Range<fpmas::api::model::GridCell>,
			public GridRangeConfig<typename _RangeConfig::Distance, typename _RangeConfig::Perimeter> {
				public:
					typedef _GridConfig GridConfig;
					typedef _RangeConfig RangeConfig;

				private:
					static const typename GridConfig::Distance grid_distance;
					static const typename RangeConfig::Distance range_distance;
					static const typename RangeConfig::Perimeter perimeter;
					DiscreteCoordinate size;

				public:
					/**
					 * GridRange constructor.
					 *
					 * @param size initial size
					 */
					GridRange(DiscreteCoordinate size)
						: size(size) {}

					/**
					 * Returns the size of this range.
					 *
					 * @return current range size
					 */
					DiscreteCoordinate getSize() const {
						return size;
					}

					/**
					 * Sets the size of this range.
					 *
					 * This can be used to dynamically update mobility or
					 * perception ranges of SpatialAgents during the simulation of
					 * a Model.
					 *
					 * @param new range size
					 */
					void setSize(DiscreteCoordinate size) {
						this->size = size;
					}

					/**
					 * Returns true if and only if the distance from
					 * `location_cell->location()` to `cell->location()`, computed
					 * using a `RangeConfig::Distance` instance, is less than or
					 * equal to the current range `size`.
					 *
					 * @param location_cell range center
					 * @param cell cell to check
					 * @return true iff `cell` is in this range
					 */
					bool contains(api::model::GridCell* location_cell, api::model::GridCell* cell) const override {
						return range_distance(location_cell->location(), cell->location()) <= size;
					}

					/**
					 * Returns the Radius of the current range, depending on the
					 * current `GridConfig` and `RangeConfig`.
					 *
					 * Possible implementation:
					 * ```cpp
					 * std::size_t radius(api::model::GridCell*) const override {
					 *     return typename GridConfig::Distance()({0, 0}, typename GridConfig::Perimeter()(*this));
					 * }
					 * ```
					 *
					 * See the fpmas::api::model::Range::radius() documentation for
					 * more details about what a "range radius" is.
					 */
					// TODO: provide a figure with examples for VN range / VN
					// grid, VN range / Moore grid, Moore range / VN grid and
					// Moore range / Moore grid.
					std::size_t radius(api::model::GridCell*) const override {
						return grid_distance({0, 0}, perimeter(*this));
					}
			};

	template<typename GridConfig, typename RangeConfig>
		const typename GridConfig::Distance GridRange<GridConfig, RangeConfig>::grid_distance;
	template<typename GridConfig, typename RangeConfig>
		const typename RangeConfig::Distance GridRange<GridConfig, RangeConfig>::range_distance;
	template<typename GridConfig, typename RangeConfig>
		const typename RangeConfig::Perimeter GridRange<GridConfig, RangeConfig>::perimeter;

	template<typename RangeType, unsigned MaxRangeSize>
		using StaticGridEndCondition = StaticEndCondition<RangeType, MaxRangeSize, api::model::GridCell>;
		
	typedef DynamicEndCondition<api::model::GridCell> DynamicGridEndCondition;
	template<
		template<typename> class SyncMode,
		typename EndCondition = DynamicGridEndCondition>
			using GridModel = SpatialModel<SyncMode, api::model::GridCell, EndCondition>;
}}

FPMAS_DEFAULT_JSON(fpmas::model::GridCell)

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2);
}}}

namespace nlohmann {
	template<typename GridCellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->_location;
			}

			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				// Initializes the current base
				derived_ptr->_location = j[1].get<fpmas::api::model::DiscretePoint>();
				return derived_ptr.get();
			}
		};

	template<typename AgentType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, Derived>>> {
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, Derived>> Ptr;
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->current_location_point;
			}

			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				// Initializes the current base
				derived_ptr->current_location_point = j[1].get<fpmas::api::model::DiscretePoint>();
				return derived_ptr.get();
			}
		};

}

namespace std {
	template<>
	struct hash<fpmas::model::DiscretePoint> {
		static const fpmas::model::PointHash hasher;
		std::size_t operator()(const fpmas::model::DiscretePoint& p) const;
	};
}
#endif
