#ifndef FPMAS_GRID_H
#define FPMAS_GRID_H

/** \file src/fpmas/model/spatial/grid.h
 * Grid models implementation.
 */

#include "fpmas/api/model/spatial/grid.h"
#include "fpmas/communication/communication.h"
#include "fpmas/random/random.h"
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
		public CellBase<api::model::GridCell, GridCellType, GridCellBase<GridCellType, Derived>> {
		friend nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<GridCellBase<GridCellType, Derived>>>;
		friend io::datapack::Serializer<fpmas::api::utils::PtrWrapper<GridCellBase<GridCellType, Derived>>>;

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
	 * @tparam AgentType final GridAgent type (i.e. the most derived type from
	 * this GridAgent)
	 * @tparam GridCellType Type of cells constituting the Grid on which the
	 * agent is moving. Must extend api::model::GridCell.
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 */
	template<typename AgentType, typename GridCellType = model::GridCell, typename Derived = AgentType>
	class GridAgent :
		public SpatialAgentBase<api::model::GridAgent<GridCellType>, AgentType, GridCellType, GridAgent<AgentType, GridCellType, Derived>> {
			friend nlohmann::adl_serializer<api::utils::PtrWrapper<GridAgent<AgentType, GridCellType, Derived>>>;
			friend io::datapack::Serializer<api::utils::PtrWrapper<GridAgent<AgentType, GridCellType, Derived>>>;
			static_assert(std::is_base_of<api::model::GridCell, GridCellType>::value,
					"The specified GridCellType must extend api::model::GridCell.");

			private:
			random::mt19937_64 _rd;
			DiscretePoint location_point {0, 0};

			protected:
			using model::SpatialAgentBase<api::model::GridAgent<GridCellType>, AgentType, GridCellType, GridAgent<AgentType, GridCellType, Derived>>::moveTo;
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

			random::mt19937_64& rd() override {
				return _rd;
			}

			void seed(std::mt19937_64::result_type seed) override {
				_rd.seed(seed);
			}
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
	 * An index mapping grid cell locations to agent indexes.
	 *
	 * The count associated to each DiscretePoint represents the count of
	 * agents located at this location.
	 */
	typedef random::Index<DiscretePoint> GridAgentIndex;

	/**
	 * Helper class that defines a static random::mt19937_64 generator used by
	 * the GridAgentBuilder class (for any `CellType`).
	 *
	 * The random generator can be seeded by the user to control random
	 * agent initialization.
	 *
	 * @see GridAgentBuilder::sample_init()
	 */
	struct RandomGridAgentBuilder {
		/**
		 * Static random generator.
		 */
		static random::mt19937_64 rd;

		/**
		 * Seeds the random generator.
		 *
		 * @param seed random seed
		 */
		static void seed(random::mt19937_64::result_type seed);
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
		class GridAgentBuilder : public SpatialAgentBuilderBase<CellType, api::model::GridCell> {
			private:
				GridAgentIndex agent_begin;
				GridAgentIndex agent_end;
				std::map<GridAgentIndex, api::model::GridAgent<CellType>*> agent_index;

			public:
				/**
				 * Implements api::model::SpatialAgentBuilder::build(SpatialModel<CellType>&, GroupList, SpatialAgentFactory<CellType>&, SpatialAgentMapping<MappingCellType>&),
				 * and seeds built agents, implicitly considered as
				 * api::model::GridAgent.
				 *
				 * The seed associated to each agent is **independent from the
				 * current cell distribution**. This notably means that all
				 * agents will generate the same random numbers sequence,
				 * independently of the current process count, the cell
				 * distribution or the migration of agents during the
				 * simulation.
				 *
				 * The process can be seeded with fpmas::seed().
				 */
				void build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups,
						api::model::SpatialAgentFactory<CellType>& factory,
						api::model::SpatialAgentMapping<api::model::GridCell>& agent_mapping
						) override;

				/**
				 * Same as build(api::model::SpatialModel<CellType>& model, api::model::GroupList groups, api::model::SpatialAgentFactory<CellType>& factory, api::model::SpatialAgentMapping<api::model::GridCell>& agent_mapping),
				 * but uses calls to `factory()` to build agents.
				 */
				void build(
						api::model::SpatialModel<CellType>& model,
						api::model::GroupList groups,
						std::function<api::model::SpatialAgent<CellType>*()> factory,
						api::model::SpatialAgentMapping<api::model::GridCell>& agent_mapping
						) override;

				/**
				 * Initializes a sample of `n` agents selected from the
				 * previously built agents with the provided `init_function`.
				 *
				 * The sample of agents selected is **deterministic**: it is
				 * guaranteed that the same agents are initialized
				 * independently of the current cell distribution.
				 *
				 * The selection process can be seeded with fpmas::seed().
				 *
				 * Successive calls can be used to independently initialize
				 * several agent states.
				 *
				 * @par Example
				 * ```cpp
				 * fpmas::model::GridAgentBuilder<> agent_builder;
				 * agent_builder.build(
				 * 	grid_model,
				 * 	{agent_group},
				 * 	[] () {return new UserAgent;},
				 * 	agent_mapping
				 * );
				 * // Sets 10 random agents to be INFECTED
				 * agent_builder.init_sample(
				 * 	10, [] (fpmas::api::model::GridAgent<>* agent) {
				 * 		((UserAgent*) agent)->setState(INFECTED);
				 * 	}
				 * );
				 * // Sets 15 random agents to be HAPPY
				 * agent_builder.init_sample(
				 * 	10, [] (fpmas::api::model::GridAgent<>* agent) {
				 * 		((UserAgent*) agent)->setMood(HAPPY);
				 * 	}
				 * );
				 * ```
				 */
				void init_sample(
						std::size_t n,
						std::function<void(api::model::GridAgent<CellType>*)> init_function
						);
		};

	template<typename CellType>
		void GridAgentBuilder<CellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups,
				api::model::SpatialAgentFactory<CellType>& factory,
				api::model::SpatialAgentMapping<api::model::GridCell>& agent_mapping
				) {
			this->build(model, groups, [&factory] () {return factory.build();}, agent_mapping);
		}

	template<typename CellType>
		void GridAgentBuilder<CellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups,
				std::function<api::model::SpatialAgent<CellType>*()> factory,
				api::model::SpatialAgentMapping<api::model::GridCell>& agent_mapping
				) {
			// Count of items by DiscretePoint (no entry <=> 0)
			std::map<DiscretePoint, std::size_t> item_counts;
			for(auto cell : model.cells()) {
				std::size_t count = agent_mapping.countAt(cell);
				if(count > 0)
					// Does not insert useless null entries
					item_counts[cell->location()]=count;
			}

			// Gather item counts from all processes
			communication::TypedMpi<decltype(item_counts)> item_counts_mpi(
					model.getMpiCommunicator());
			item_counts =
				communication::all_reduce(item_counts_mpi, item_counts, utils::Concat());

			// Contains the cumulative sum of item_counts entries
			std::map<DiscretePoint, std::size_t> item_counts_sum;

			// Loop ordered by DiscretePoint
			std::size_t sum = 0;
			for(auto item : item_counts) {
				sum+=item.second;
				item_counts_sum.insert({item.first, sum});
			}
			// Containes the current index of agents located at each location
			std::map<DiscretePoint, std::size_t> local_counts;

			// Built agents
			std::vector<api::model::GridAgent<CellType>*> agents;

			this->build_agents(model, groups, [&agents, &factory] {
					auto agent = factory();
					// Keep a reference to built agents
					agents.push_back((api::model::GridAgent<CellType>*) agent);
					return agent;
					}, agent_mapping);

			agent_begin = GridAgentIndex::begin(item_counts);
			agent_end = GridAgentIndex::end(item_counts);
			std::vector<std::mt19937_64::result_type> local_seeds(
					GridAgentIndex::distance(agent_begin, agent_end)
					);
			for(std::size_t i = 0; i < local_seeds.size(); i++)
				// Generates a unique integer for each, independently from the
				// current distribution
				local_seeds[i] = i;
			// Genererates a better seed distribution. The same seeds are
			// generated on all processes.
			std::seed_seq seed_generator(local_seeds.begin(), local_seeds.end());
			seed_generator.generate(local_seeds.begin(), local_seeds.end());

			for(std::size_t i = 0; i < agents.size(); i++) {
				std::size_t& offset = local_counts[agents[i]->locationPoint()];
				GridAgentIndex index(item_counts, agents[i]->locationPoint(), offset);
				agent_index.insert({index, agents[i]});
				agents[i]->seed(
						local_seeds[GridAgentIndex::distance(agent_begin, index)]
						);

				++offset;
			}
		}

	template<typename CellType>
		void GridAgentBuilder<CellType>::init_sample(
				std::size_t n,
				std::function<void(api::model::GridAgent<CellType>*)> init_function) {
			std::vector<GridAgentIndex> indexes = random::sample_indexes(
					agent_begin, agent_end, n, RandomGridAgentBuilder::rd);
			for(auto index : indexes) {
				auto it = agent_index.find(index);
				if(it != agent_index.end())
					init_function(it->second);
			}
		}

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
FPMAS_DEFAULT_DATAPACK(fpmas::model::GridCell)

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
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->location_point;
				j[2] = ptr->_rd;
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
				derived_ptr->_rd = j[2].get<fpmas::random::mt19937_64>();
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
		struct light_serializer<PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridCellBase.
			 */
			typedef PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;
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
				light_serializer<PtrWrapper<Derived>>::to_json(
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
				PtrWrapper<Derived> derived_ptr
					= light_serializer<PtrWrapper<Derived>>::from_json(j);
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
		struct light_serializer<PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridAgent.
			 */
			typedef PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>> Ptr;

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
				light_serializer<PtrWrapper<Derived>>::to_json(
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
				PtrWrapper<Derived> derived_ptr
					= light_serializer<PtrWrapper<Derived>>::from_json(j);
				return derived_ptr.get();
			}
		};

}}}

namespace fpmas { namespace io { namespace datapack {
	/**
	 * Polymorphic GridCellBase ObjectPack serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` ObjectPack serialization | GridCellBase::location() |
	 *
	 * The `Derived` part is serialized using the
	 * Serializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint. The input GridCellBase pointer
	 * is cast to `Derived` when required to call the proper Serializer
	 * specialization.
	 *
	 * @tparam GridCellType final fpmas::api::model::GridCell type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename GridCellType, typename Derived>
		struct Serializer<PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridCellBase.
			 */
			typedef PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the GridCellBase
			 * pointed by `ptr` in `p`.
			 */
			static std::size_t size(const ObjectPack& p, const Ptr& ptr) {
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				return p.size(derived) + p.size<api::model::DiscretePoint>();
			}

			/**
			 * Serializes the pointer to the polymorphic GridCellBase into the
			 * specified ObjectPack.
			 *
			 * @param pack destination ObjectPack
			 * @param ptr pointer to a polymorphic GridAgent to serialize
			 */
			static void to_datapack(ObjectPack& pack, const Ptr& ptr) {
				// Derived serialization
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				pack.put(derived);
				// Current base serialization (only location is needed)
				pack.put(ptr->_location);
			}

			/**
			 * Unserializes a polymorphic GridCellBase from the specified
			 * ObjectPack.
			 *
			 * First, the `Derived` part, that extends `GridCellBase` by
			 * definition, is unserialized from the ObjectPack using the
			 * `Serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GridAgent` members
			 * undefined. The specific `GridAgent` member `location_point` is
			 * then initialized from the second ObjectPack field, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `GridCellBase` pointer.
			 *
			 * @param pack source ObjectPack
			 * @return unserialized pointer to a polymorphic `GridCellBase`
			 */
			static Ptr from_datapack(const ObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr = pack
					.get<PtrWrapper<Derived>>();

				// Initializes the current base
				derived_ptr->_location = pack.get<fpmas::api::model::DiscretePoint>();
				return derived_ptr.get();
			}
		};

	/**
	 * Polymorphic GridAgent ObjectPack serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` ObjectPack serialization | GridAgent::locationPoint() |
	 *
	 * The `Derived` part is serialized using the
	 * Serializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint. The input GridAgent pointer is
	 * cast to `Derived` when required to call the proper Serializer
	 * specialization.
	 *
	 * @tparam AgentType final fpmas::api::model::GridAgent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct Serializer<PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridAgent.
			 */
			typedef PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * GridAgent pointed by `ptr` into `p`.
			 */
			static std::size_t size(const ObjectPack& p, const Ptr& ptr) {
				return p.size(PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get()))))
					+ p.size<fpmas::api::model::DiscretePoint>()
					+ p.size(ptr->_rd);
			}

			/**
			 * Serializes the pointer to the polymorphic GridAgent into the
			 * specified ObjectPack.
			 *
			 * @param pack destination ObjectPack
			 * @param ptr pointer to a polymorphic GridAgent to serialize
			 */
			static void to_datapack(ObjectPack& pack, const Ptr& ptr) {
				// Derived serialization
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				pack.put(derived);
				// Current base serialization
				pack.put(ptr->location_point);
				pack.put(ptr->_rd);
			}

			/**
			 * Unserializes a polymorphic GridAgent from the specified
			 * ObjectPack.
			 *
			 * First, the `Derived` part, that extends `GridAgent` by
			 * definition, is unserialized from the ObjectPack using the
			 * `Serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GridAgent` members
			 * undefined. The specific `GridAgent` member `location_point` is
			 * then initialized from the second ObjectPack field, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `GridAgent` pointer.
			 *
			 * @param pack source ObjectPack
			 * @return unserialized pointer to a polymorphic `GridAgent`
			 */
			static Ptr from_datapack(const ObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr = pack
					.get<PtrWrapper<Derived>>();

				// Initializes the current base
				derived_ptr->location_point =
					pack.get<fpmas::api::model::DiscretePoint>();
				derived_ptr->_rd = pack.get<fpmas::random::mt19937_64>();
				return derived_ptr.get();
			}
		};

	/**
	 * LightSerializer specialization for an fpmas::model::GridCellBase
	 *
	 * The LightSerializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current LightObjectPack.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` LightObjectPack serialization |
	 *
	 * The `Derived` part is serialized using the
	 * LightSerializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint (and potentially leaves the
	 * LightObjectPack empty).
	 *
	 * @tparam GridCellType final fpmas::api::model::GridCell type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename GridCellType, typename Derived>
		struct LightSerializer<PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridCellBase.
			 */
			typedef PtrWrapper<fpmas::model::GridCellBase<GridCellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * GridCellBase pointed by `ptr` into `p`, i.e. the buffer size
			 * required to serialize the `Derived` part of `ptr`.
			 */
			static std::size_t size(const LightObjectPack& p, const Ptr& ptr) {
				return p.size(PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get()))
						));
			}

			/**
			 * Serializes the pointer to the polymorphic GridCellBase into the
			 * specified LightObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_datapack()`,
			 * without adding any `GridCellBase` specific data to the
			 * LightObjectPack.
			 *
			 * @param pack destination LightObjectPack
			 * @param cell grid cell to serialize 
			 */
			static void to_datapack(LightObjectPack& pack, const Ptr& cell) {
				// Derived serialization
				LightSerializer<PtrWrapper<Derived>>::to_datapack(
						pack,
						const_cast<Derived*>(static_cast<const Derived*>(cell.get()))
						);
			}

			/**
			 * Unserializes a polymorphic GridCellBase from the specified
			 * LightObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_datapack()`,
			 * without extracting any `GridCellBase` specific data from the
			 * LightObjectPack.
			 *
			 * @param pack LightObjectPack
			 * @return dynamically allocated `Derived` instance, unserialized from `pack`
			 */
			static Ptr from_datapack(const LightObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr
					= LightSerializer<PtrWrapper<Derived>>::from_datapack(pack);
				return derived_ptr.get();
			}
		};

	/**
	 * LightSerializer specialization for an fpmas::model::GridAgent
	 *
	 * The LightSerializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current LightObjectPack.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` LightObjectPack serialization |
	 *
	 * The `Derived` part is serialized using the
	 * LightSerializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint (and potentially leaves the
	 * LightObjectPack empty).
	 *
	 * @tparam AgentType final fpmas::api::model::GridAgent type to serialize
	 * @tparam CellType type of cells used by the spatial model
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename CellType, typename Derived>
		struct LightSerializer<PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic GridAgent.
			 */
			typedef PtrWrapper<fpmas::model::GridAgent<AgentType, CellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * GridAgent pointed by `ptr` into `p`, i.e. the buffer size
			 * required to serialize the `Derived` part of `ptr`.
			 */
			static std::size_t size(const LightObjectPack& p, const Ptr& ptr) {
				return p.size(PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get()))
						));
			}

			/**
			 * Serializes the pointer to the polymorphic GridAgent into the
			 * specified LightObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_datapack()`,
			 * without adding any `GridAgent` specific data to the
			 * LightObjectPack.
			 *
			 * @param pack destination LightObjectPack
			 * @param agent grid agent to serialize 
			 */
			static void to_datapack(LightObjectPack& pack, const Ptr& agent) {
				// Derived serialization
				LightSerializer<PtrWrapper<Derived>>::to_datapack(
						pack,
						const_cast<Derived*>(static_cast<const Derived*>(agent.get()))
						);
			}

			/**
			 * Unserializes a polymorphic GridAgent from the specified
			 * LightObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_datapack()`,
			 * without extracting any `GridAgent` specific data from the
			 * LightObjectPack.
			 *
			 * @param pack source LightObjectPack
			 * @return dynamically allocated `Derived` instance, unserialized from `pack`
			 */
			static Ptr from_datapack(const LightObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr
					= LightSerializer<PtrWrapper<Derived>>::from_datapack(pack);
				return derived_ptr.get();
			}
		};

	/**
	 * DiscretePoint base_io specialization.
	 *
	 * | Serialization scheme ||
	 * | point.x | point.y |
	 */
	template<>
		struct Serializer<api::model::DiscretePoint> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a
			 * DiscretePoint instance in a DataPack, i.e.
			 * `2*%p.size<DiscreteCoordinate>()`.
			 */
			static std::size_t size(const ObjectPack& p);

			/**
			 * Equivalent to size().
			 */
			static std::size_t size(const ObjectPack& p, const api::model::DiscretePoint&);

			/**
			 * Writes `id` to the `pack` buffer.
			 *
			 * @param pack destination ObjectPack
			 * @param point source point
			 */
			static void to_datapack(
					ObjectPack& pack, const api::model::DiscretePoint& point);

			/**
			 * Reads a DiscretePoint from the `pack` buffer.
			 *
			 * @param pack source ObjectPack
			 * @return read DiscretePoint
			 */
			static api::model::DiscretePoint from_datapack(const ObjectPack& pack);
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
