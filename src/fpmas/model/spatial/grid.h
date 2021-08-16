#ifndef FPMAS_GRID_H
#define FPMAS_GRID_H

/** \file src/fpmas/model/spatial/grid.h
 * Grid models implementation.
 */

#include "fpmas/api/model/spatial/grid.h"
#include "spatial_model.h"
#include "fpmas/random/distribution.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * DiscretePoint hash function, inspired from
	 * [boost::hash_combine](https://www.boost.org/doc/libs/1_37_0/doc/html/hash/reference.html#boost.hash_combine).
	 */
	struct PointHash {
		/**
		 * Computes an hash value for `p` using the following method:
		 * ```cpp
		 * std::size_t hash = std::hash<DiscreteCoordinate>()(p.x);
		 * hash ^= std::hash<DiscreteCoordinate>()(p.y) + 0x9e3779b9
		 *     + (hash << 6) + (hash >> 2);
		 * ```
		 *
		 * This comes directly from the
		 * [boost::hash_combine](https://www.boost.org/doc/libs/1_37_0/doc/html/hash/reference.html#boost.hash_combine)
		 * method.
		 *
		 * @param p discrete point to hash
		 * @return hash value for `p`
		 */
		std::size_t operator()(const DiscretePoint& p) const;
	};
		
	/**
	 * api::model::GridCell implementation.
	 *
	 * @tparam GridCellType dynamic GridCellBase type (i.e. the most derived
	 * type from this GridCellBase)
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 */
	template<typename GridCellType, typename Derived = GridCellType>
	class GridCellBase :
		public virtual api::model::GridCell,
		public Cell<GridCellType, GridCellBase<GridCellType, Derived>> {
		friend nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<GridCellBase<GridCellType, Derived>>>;
		friend fpmas::io::json::light_serializer<fpmas::api::utils::PtrWrapper<GridCellBase<GridCellType, Derived>>>;

		private:
			DiscretePoint _location;
		public:
			/**
			 * Type that must be registered to enable Json serialization.
			 */
			typedef GridCellBase<GridCellType, Derived> JsonBase;

			/**
			 * Default GridCellBase constructor.
			 */
			GridCellBase() {}

			/**
			 * GridCellBase constructor.
			 *
			 * @param location grid cell coordinates
			 */
			GridCellBase(DiscretePoint location)
				: _location(location) {}

			/**
			 * \copydoc fpmas::api::model::GridCell::location
			 */
			DiscretePoint location() const override {
				return _location;
			}
	};

	/**
	 * Default GridCell type, without any user defined behavior or
	 * serialization rules.
	 */
	class GridCell : public GridCellBase<GridCell> {
		public:
			using GridCellBase<GridCell>::GridCellBase;
	};

	/**
	 * api::model::GridAgent implementation.
	 *
	 * @tparam AgentType dynamic GridAgent type (i.e. the most derived
	 * type from this GridAgent)
	 * @tparam GridCellType Type of cells constituting the Grid on which the
	 * agent is moving. Must extend api::model::GridCell.
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 */
	template<typename AgentType, typename GridCellType = model::GridCell, typename Derived = AgentType>
	class GridAgent :
		public virtual api::model::GridAgent<GridCellType>,
		public SpatialAgentBase<AgentType, GridCellType, GridAgent<AgentType, GridCellType, Derived>> {
			friend nlohmann::adl_serializer<api::utils::PtrWrapper<GridAgent<AgentType, GridCellType, Derived>>>;
			static_assert(std::is_base_of<api::model::GridCell, GridCellType>::value,
					"The specified GridCellType must extend api::model::GridCell.");

			private:
			DiscretePoint location_point {0, 0};

			protected:
			using model::SpatialAgentBase<AgentType, GridCellType, GridAgent<AgentType, GridCellType, Derived>>::moveTo;
			/**
			 * \copydoc fpmas::api::model::GridAgent::moveTo(GridCellType*)
			 */
			void moveTo(GridCellType* cell) override;
			/**
			 * \copydoc fpmas::api::model::GridAgent::moveTo(DiscretePoint)
			 */
			void moveTo(DiscretePoint point) override;

			public:
			/**
			 * \copydoc fpmas::api::model::GridAgent::locationPoint
			 */
			DiscretePoint locationPoint() const override {return location_point;}
		};

	template<typename AgentType, typename GridCellType, typename Derived>
		void GridAgent<AgentType, GridCellType, Derived>::moveTo(GridCellType* cell) {
			this->updateLocation(cell);
			location_point = cell->location();
		}

	template<typename AgentType, typename GridCellType, typename Derived>
		void GridAgent<AgentType, GridCellType, Derived>::moveTo(DiscretePoint point) {
			bool found = false;
			auto mobility_field = this->template outNeighbors<GridCellType>(SpatialModelLayers::MOVE);
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

	/**
	 * api::model::GridCellFactory implementation.
	 *
	 * @tparam GridCellType type of cells to build. A custom user defined grid
	 * cell type can be provided.
	 */
	template<typename GridCellType = GridCell>
		class GridCellFactory : public api::model::GridCellFactory<GridCellType> {
			public:
				/**
				 * Build a new api::model::GridCell using the following
				 * statement:
				 * ```cpp
				 * new GridCellType(location)
				 * ```
				 * @param location grid cell coordinates
				 */
				GridCellType* build(DiscretePoint location) override {
					return new GridCellType(location);
				}
		};

	/**
	 * A GridCellFactory specialization for api::model::GridCell.
	 *
	 * GridCellFactory normally accepts constructible types as the
	 * `GridCellType` template parameter. This specialization can however by
	 * used to produce default GridCell instances.
	 */
	template<>
		class GridCellFactory<api::model::GridCell> : public api::model::GridCellFactory<api::model::GridCell>{
			api::model::GridCell* build(DiscretePoint location) override {
				return new GridCell(location);
			}
		};

	/**
	 * Grid specialization of the SpatialAgentBuilder class.
	 *
	 * The fpmas::model::GridAgentBuilder uses api::model::GridCell as the `MappingCellType`
	 * parameter, meaning an
	 * SpatialAgentMapping<api::model::GridCell> must be provided
	 * to the fpmas::model::GridAgentBuilder::build() method. As an example,
	 * RandomGridAgentMapping might be used. 
	 */
	template<typename CellType = GridCell>
	using GridAgentBuilder = SpatialAgentBuilder<CellType, api::model::GridCell>;


	/**
	 * Generic static Grid configuration interface.
	 */
	template<typename BuilderType, typename DistanceType, typename _CellType = model::GridCell>
		struct GridConfig {
			/**
			 * fpmas::api::model::CellNetworkBuilder implementation that can
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

			/**
			 * Type of fpmas::api::model::Cell used by the Grid.
			 *
			 * This type is notably used to define Range types of GridAgent
			 * evolving on the Grid.
			 */
			typedef _CellType CellType;
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
	 * threshold and the `GridRangeConfig::Distance` function object.
	 *
	 * Formally a GridRange `range` centered on `p1` is constituted by any
	 * point of the Grid `p` such that `GridRangeConfig::Distance()(p1, p) <=
	 * range.getSize()`.
	 *
	 * Notice the two following cases:
	 * - if `size==0`, only the current location is included in the range.
	 * - if `size<0`, the range is empty.
	 *
	 * @tparam GridConfig grid configuration, that notably defines how a
	 * distance between two cells of the grid can be computed, using the
	 * GridConfig::Distance function object. The type GridConfig::CellType
	 * also defines which type of cells are used to define the Grid.
	 *
	 * @tparam GridRangeConfig grid range configuration. The
	 * GridRangeConfig::Distance member type is used to define the shape of the
	 * range, independently of the current GridConfig. The
	 * GridRangeConfig::Perimeter member type is used to define the perimeter
	 * of the range.
	 */
	template<typename GridConfig, typename GridRangeConfig>
		class GridRange : public api::model::Range<typename GridConfig::CellType> {
			private:
				static const typename GridConfig::Distance grid_distance;
				static const typename GridRangeConfig::Distance range_distance;
				static const typename GridRangeConfig::Perimeter perimeter;
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
				 * @param size new range size
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
				bool contains(
						typename GridConfig::CellType* location_cell,
						typename GridConfig::CellType* cell) const override {
					if(this->getSize() < 0)
						return false;
					else
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
				std::size_t radius(typename GridConfig::CellType*) const override {
					return grid_distance({0, 0}, perimeter(*this));
				}
		};

	template<typename GridConfig, typename RangeConfig>
		const typename GridConfig::Distance GridRange<GridConfig, RangeConfig>::grid_distance;
	template<typename GridConfig, typename RangeConfig>
		const typename RangeConfig::Distance GridRange<GridConfig, RangeConfig>::range_distance;
	template<typename GridConfig, typename RangeConfig>
		const typename RangeConfig::Perimeter GridRange<GridConfig, RangeConfig>::perimeter;

	/**
	 * Grid specialization of the SpatialModel.
	 */
	template<
		template<typename> class SyncMode,
		typename CellType = model::GridCell,
		typename EndCondition = DynamicEndCondition<CellType>>
			using GridModel = SpatialModel<SyncMode, CellType, EndCondition>;
}}

FPMAS_DEFAULT_JSON(fpmas::model::GridCell)

namespace fpmas { namespace api { namespace model {
	/**
	 * Checks the equality of two DiscretePoints.
	 *
	 * `p1` and `p2` are equals if and only if their two coordinates are equal.
	 *
	 * @return true iff `p1` and `p2` are equal
	 */
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2);
}}}

namespace nlohmann {
	/**
	 * nlohmann::adl_serializer specialization for
	 * fpmas::api::model::DiscretePoint.
	 */
	template<>
		struct adl_serializer<fpmas::api::model::DiscretePoint> {
			/**
			 * Serializes the specified point as `[<json.x>, <json.y>]`.
			 *
			 * @param j output json
			 * @param point point to serialize
			 */
			static void to_json(nlohmann::json& j, const fpmas::api::model::DiscretePoint& point) {
				j = {point.x, point.y};
			}
			/**
			 * Unserializes a DiscretePoint from the input json.
			 *
			 * The input json must have the form `[x, y]`.
			 *
			 * @param j input json
			 * @param point unserialized point
			 */
			static void from_json(const nlohmann::json& j, fpmas::api::model::DiscretePoint& point) {
				point.x = j[0].get<fpmas::api::model::DiscreteCoordinate>();
				point.y = j[1].get<fpmas::api::model::DiscreteCoordinate>();
			}
		};

	/**
	 * Polymorphic GridCellBase nlohmann json serializer specialization.
	 */
	template<typename GridCellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridCellBase.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;

			/**
			 * Serializes the pointer to the polymorphic GridCellBase using
			 * the following JSON schema:
			 * ```json
			 * [<Derived json serialization>, ptr->location()]
			 * ```
			 *
			 * The `<Derived json serialization>` is computed using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>>`
			 * specialization, that can be user defined without additional
			 * constraint.
			 *
			 * @param j json output
			 * @param ptr pointer to a polymorphic GridAgent to serialize
			 */
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->_location;
			}

			/**
			 * Unserializes a polymorphic GridCellBase from the specified Json.
			 *
			 * First, the `Derived` part, that extends `GridCellBase` by
			 * definition, is unserialized from `j[0]` using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization, that can be defined externally without any
			 * constraint. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GridAgent`
			 * members undefined. The specific `GridAgent` member
			 * `location_point` is then initialized from `j[1]`, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `GridCellBase` pointer.
			 *
			 * @param j json input
			 * @return unserialized pointer to a polymorphic `GridCellBase`
			 */
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

	/**
	 * Polymorphic GridAgent nlohmann json serializer specialization.
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridAgent.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>> Ptr;

			/**
			 * Serializes the pointer to the polymorphic GridAgent using
			 * the following JSON schema:
			 * ```json
			 * [<Derived json serialization>, ptr->locationPoint()]
			 * ```
			 *
			 * The `<Derived json serialization>` is computed using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>>`
			 * specialization, that can be user defined without additional
			 * constraint.
			 *
			 * @param j json output
			 * @param ptr pointer to a polymorphic GridAgent to serialize
			 */
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->location_point;
			}

			/**
			 * Unserializes a polymorphic GridAgent from the specified Json.
			 *
			 * First, the `Derived` part, that extends `GridAgent` by
			 * definition, is unserialized from `j[0]` using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization, that can be defined externally without any
			 * constraint. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GridAgent`
			 * members undefined. The specific `GridAgent` member
			 * `location_point` is then initialized from `j[1]`, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `GridAgent` pointer.
			 *
			 * @param j json input
			 * @return unserialized pointer to a polymorphic `GridAgent`
			 */
			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				// Initializes the current base
				derived_ptr->location_point = j[1].get<fpmas::api::model::DiscretePoint>();
				return derived_ptr.get();
			}
		};

}

namespace fpmas { namespace io { namespace json {

	/**
	 * light_serializer specialization for an fpmas::model::GridCellBase
	 *
	 * The light_serializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current \light_json.
	 *
	 * @tparam GridCellType final fpmas::api::model::GridCell type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename GridCellType, typename Derived>
		struct light_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridCellBase.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;
			/**
			 * \light_json to_json() implementation for an
			 * fpmas::model::GridCellBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_json()`,
			 * without adding any `GridCellBase` specific data to the
			 * \light_json j.
			 *
			 * @param j json output
			 * @param cell grid cell to serialize 
			 */
			static void to_json(light_json& j, const Ptr& cell) {
				// Derived serialization
				light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::to_json(
						j,
						const_cast<Derived*>(static_cast<const Derived*>(cell.get()))
						);
			}

			/**
			 * \light_json from_json() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_json()`,
			 * without extracting any `GridCellBase` specific data from the
			 * \light_json j.
			 *
			 * @param j json input
			 * @return dynamically allocated `Derived` instance, unserialized from `j`
			 */
			static Ptr from_json(const light_json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::from_json(j);
				return derived_ptr.get();
			}
		};

	/**
	 * light_serializer specialization for an fpmas::model::GridAgent
	 *
	 * The light_serializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current \light_json.
	 *
	 * @tparam AgentType final \Agent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct light_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridAgent.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>> Ptr;

			/**
			 * \light_json to_json() implementation for an
			 * fpmas::model::GridAgent.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_json()`,
			 * without adding any `GridAgent` specific data to the \light_json
			 * j.
			 *
			 * @param j json output
			 * @param agent grid agent to serialize 
			 */
			static void to_json(light_json& j, const Ptr& agent) {
				// Derived serialization
				light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::to_json(
						j,
						const_cast<Derived*>(static_cast<const Derived*>(agent.get()))
						);
			}

			/**
			 * \light_json from_json() implementation for an
			 * fpmas::model::GridAgent.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_json()`,
			 * without extracting any `GridAgent` specific data from the
			 * \light_json j.
			 *
			 * @param j json input
			 * @return dynamically allocated `Derived` instance, unserialized from `j`
			 */
			static Ptr from_json(const light_json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::from_json(j);
				return derived_ptr.get();
			}
		};

}}}

namespace std {
	/**
	 * fpmas::model::DiscretePoint hash function object.
	 */
	template<>
	struct hash<fpmas::model::DiscretePoint> {
		/**
		 * Static fpmas::model::PointHash instance.
		 */
		static const fpmas::model::PointHash hasher;
		/**
		 * Returns hasher(p).
		 *
		 * @param p point to hash
		 * @return point hash
		 */
		std::size_t operator()(const fpmas::model::DiscretePoint& p) const;
	};
}
#endif
