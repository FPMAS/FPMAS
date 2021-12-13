#ifndef FPMAS_MODEL_SPATIAL_GRAPH_H
#define FPMAS_MODEL_SPATIAL_GRAPH_H

/**
 * \file src/fpmas/model/spatial/graph.h
 *
 * Defines features that can be used to build spatial graph based models, i.e.
 * models where agents are moving on an arbitrary graph.
 */
#include "spatial_model.h"

namespace fpmas { namespace model {

	/**
	 * A cell base class that can be used to implement SpatialModels based on a
	 * generic and abstract graph.
	 *
	 * A #reachable_cells collection is maintained to specify direct neighbors
	 * of this Cell, even if the Cell is DISTANT from the current process.
	 *
	 * As a reminder, the neighbor list of a DISTANT node can't be accessed
	 * directly within a DistributedGraph instance.
	 */
	class ReachableCell {
		friend nlohmann::adl_serializer<ReachableCell>;
		friend io::datapack::Serializer<ReachableCell>;

		protected:
			/**
			 * Ids of direct neighbors of this cell on the CELL_SUCCESSOR
			 * layer.
			 *
			 * This collection must be built with addReachableCell().
			 */
			std::set<DistributedId> reachable_cells;

		public:

			/**
			 * Returns a set containing ids of cells reachable from this cell.
			 */
			const std::set<DistributedId>& reachableCells() const {
				return reachable_cells;
			}

			/**
			 * Adds the provided id to the #reachable_cells collection.
			 *
			 * @param id id of a cell reachable from this cell
			 */
			void addReachableCell(DistributedId id) {
				reachable_cells.insert(id);
			}

			/**
			 * Removes the provided if from the #reachable_cells collection.
			 *
			 * @param id id of a cell previously reachable from this cell
			 */
			void removeReachableCell(DistributedId id) {
				reachable_cells.erase(id);
			}

			/**
			 * Clears the #reachable_cells collection.
			 */
			void clearReachableCells() {
				reachable_cells.clear();
			}
	};

	/**
	 * A basic Cell implementation that can be used to build \SpatialModels
	 * based on a generic graph structure.
	 *
	 * @tparam GraphCellType final GraphCellBase implementation
	 * @tparam Derived direct derived class, or at least the next class in the
	 * serialization chain
	 *
	 * @see GraphCell
	 */
	template<typename GraphCellType, typename Derived = GraphCellType>
		class GraphCellBase :
			public Cell<GraphCellType, GraphCellBase<GraphCellType, Derived>>,
			public ReachableCell {
				public:
				/**
				 * Type that must be registered to enable Json serialization.
				 */
				typedef GraphCellBase<GraphCellType, Derived> JsonBase;
			};

	/**
	 * A final GraphCell type, that can be used directly in a SpatialModel.
	 */
	struct GraphCell final : public GraphCellBase<GraphCell> {
		using GraphCellBase<GraphCell>::GraphCellBase;
	};

	/**
	 * A GraphCell based api::model::Range implementation.
	 *
	 * The specified GraphCellType is assumed to extend the two following
	 * classes:
	 * 1. api::model::Cell
	 * 2. ReachableCell
	 *
	 * A `cell` is assumed to be contained in this GraphRange, centered in
	 * `root`, if and only if the `cell` id is contained in the
	 * `reachable_cells` list of the `root`.
	 *
	 * It is currently assumed that ids contained in the `reachable_cells` list
	 * are **direct neighbors** of the `root` on the `CELL_SUCCESSOR`, what
	 * means that a `cell` at a distance of 2 (in term of graph distance) from
	 * `root` can't be added to the range. This behavior might evolve in the
	 * future.
	 *
	 * The special case `cell==root` is handled according to the
	 * `include_location` parameter, specified in the constructor.
	 *
	 * In order to be usable, is it **required** to call the synchronize()
	 * method after the corresponding SpatialModel initialization, but before
	 * any SpatialAgent execution. This requirement will probably be relaxed in
	 * a next release.
	 */
	/*
	 * TODO: How to avoid synchronize()?
	 * Event listeners might be added to GraphCells to automatically update the
	 * `reachable_cells` list when a GraphCell is linked / unlinked on the
	 * CELL_SUCCESSOR layer. However, such event handling is not yet supported
	 * by FPMAS 1.1.
	 */
	template<typename GraphCellType = GraphCell>
		class GraphRange : public api::model::Range<GraphCellType> {
			static_assert(
					std::is_base_of<api::model::Cell, GraphCellType>::value,
					"The specified GraphCellType must implement api::model::Cell."
					);
			static_assert(
					std::is_base_of<ReachableCell, GraphCellType>::value,
					"The specified GraphCellType must extend ReachableCell"
					);
			private:
				bool include_location;
			public:
				/**
				 * GraphRange constructor.
				 *
				 * @param include_location specifies if the current location of
				 * the SpatialAgent (assumed to be `root`) must be included in
				 * this GraphRange
				 */
				GraphRange(bool include_location = INCLUDE_LOCATION) : include_location(include_location) {
				}

				/**
				 * Radius of the GraphRange.
				 *
				 * Since it is assumed that only direct neighbors of a `root`
				 * on the CELL_SUCCESSOR layer can be added to its
				 * `reachable_cells` collection, the radius of this range is 1.
				 *
				 * @return radius of this GraphRange (always 1)
				 */
				std::size_t radius(GraphCellType*) const override {
					return 1;
				}

				/**
				 * If `root==cell`:
				 *   - returns `true` iff `include_location` is true.
				 * Else:
				 *   - returns `true` iff the `cell` id is contained in the
				 *   `reachable_cells` list of `root`
				 *
				 * @return true iff `cell` is contained in this GraphRange,
				 * located on `root`.
				 */
				bool contains(GraphCellType* root, GraphCellType* cell) const override {
					if(include_location && root->node()->getId() == cell->node()->getId())
						return true;
					return root->reachableCells().count(cell->node()->getId()) > 0;
				}

				/**
				 * Automatically updates the `reachable_cells` list of GraphCells
				 * contained in the specified `model`.
				 *
				 * All out neighbors cells on the `CELL_SUCCESSOR` layer of
				 * each GraphCell are added to its `reachable_cells` list.
				 *
				 * The synchronization can be performed dynamically if the
				 * GraphCell network evolves during the simulation.
				 *
				 * This method is synchronous, and must be called from all
				 * processes.
				 *
				 * @param model initialized SpatialModel, based on a GraphCell
				 * network.
				 */
				static void synchronize(api::model::SpatialModel<GraphCellType>& model) {
					for(auto cell : model.cellGroup().localAgents()) {
						auto graph_cell = dynamic_cast<ReachableCell*>(cell);
						graph_cell->clearReachableCells();
						for(auto neighbor : cell->node()->outNeighbors(api::model::CELL_SUCCESSOR))
							graph_cell->addReachableCell(neighbor->getId());
					}
					model.graph().synchronize();
				}

		};
}}

FPMAS_DEFAULT_JSON(fpmas::model::GraphCell)
FPMAS_DEFAULT_DATAPACK(fpmas::model::GraphCell)

namespace nlohmann {
	/**
	 * fpmas::model::ReachableCell nlohmann json serializer specialization.
	 */
	template<>
		struct adl_serializer<fpmas::model::ReachableCell> {
			/**
			 * Serializes the specified `cell` into the json `j`.
			 *
			 * @param j json output
			 * @param cell reachable cell to serialize
			 */
			static void to_json(nlohmann::json& j, const fpmas::model::ReachableCell& cell);
			/**
			 * Unserializes a `cell` from the json `j`.
			 *
			 * @param j input json
			 * @param cell reachable cell output
			 */
			static void from_json(const nlohmann::json& j, fpmas::model::ReachableCell& cell);
		};

	/**
	 * Polymorphic GraphCellBase nlohmann json serializer specialization.
	 */
	template<typename AgentType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GraphCellBase<AgentType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GraphCellBase<AgentType, Derived>> Ptr;
			/**
			 * Serializes the pointer to the polymorphic SpatialAgentBase using
			 * the following JSON schema:
			 * ```json
			 * [<Derived json serialization>, ptr->reachableNodes()]
			 * ```
			 *
			 * The `<Derived json serialization>` is computed using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>>`
			 * specialization, that can be defined externally without
			 * additional constraint.
			 *
			 * @param j json output
			 * @param ptr pointer to a polymorphic SpatialAgentBase to serialize
			 */
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				j[1] = static_cast<const fpmas::model::ReachableCell&>(*ptr);
			}

			/**
			 * Unserializes a polymorphic GraphCellBase 
			 * from the specified Json.
			 *
			 * First, the `Derived` part, that extends `GraphCellBase` by
			 * definition, is unserialized from `j[0]` using the
			 * `adl_serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization, that can be defined externally without any
			 * constraint. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GraphCellBase`
			 * members undefined. The specific `GraphCellBase` member
			 * `reachable_nodes` is then initialized from `j[1]`, and the
			 * unserialized `Derived` instance is returned in the form of a
			 * polymorphic `GraphCellBase` pointer.
			 *
			 * @param j json input
			 * @return unserialized pointer to a polymorphic `GraphCellBase`
			 */
			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();
				static_cast<fpmas::model::ReachableCell&>(*derived_ptr)
					= j[1].get<fpmas::model::ReachableCell>();
				//derived_ptr->reachable_nodes = j[1].get<std::set<fpmas::api::graph::DistributedId>>();
				return derived_ptr.get();
			}
		};
}

namespace fpmas { namespace io { namespace json {
	/**
	 * light_serializer specialization for an fpmas::model::GraphCellBase
	 *
	 * The light_serializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current \light_json.
	 *
	 * @tparam AgentType final \Agent type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename AgentType, typename Derived>
		struct light_serializer<PtrWrapper<fpmas::model::GraphCellBase<AgentType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::GraphCellBase<AgentType, Derived>> Ptr;
			
			/**
			 * \light_json to_json() implementation for an
			 * fpmas::model::SpatialAgentBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_json()`,
			 * without adding any `GraphCellBase` specific data to the
			 * \light_json j.
			 *
			 * @param j json output
			 * @param agent agent to serialize 
			 */
			static void to_json(light_json& j, const Ptr& agent) {
				// Derived serialization
				light_serializer<PtrWrapper<Derived>>::to_json(
						j,
						const_cast<Derived*>(dynamic_cast<const Derived*>(agent.get()))
						);
			}

			/**
			 * \light_json from_json() implementation for an
			 * fpmas::model::GraphCellBase.
			 *
			 * Effectively calls
			 * `light_serializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_json()`,
			 * without extracting any `GraphCellBase` specific data from the
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
	 * fpmas::model::ReachableCell ObjectPack serializer specialization.
	 *
	 * | Serialization Scheme |
	 * |----------------------|
	 * | cell.reachable_cells |
	 */
	template<>
		struct Serializer<model::ReachableCell> {
			/**
			 * Returns the buffer size, in bytes, required to serialize a
			 * model::ReachableCell instance, i.e.
			 * `p.size(cell.reachable_cells)`.
			 *
			 * @return pack size in bytes
			 */
			static std::size_t size(
					const ObjectPack& p, const model::ReachableCell& cell);

			/**
			 * Serializes `cell` into `pack`.
			 *
			 * @param pack destination ObjectPack
			 * @param cell ReachableCell to serialize
			 */
			static void to_datapack(
					ObjectPack& pack, const model::ReachableCell& cell);

			/**
			 * Unserializes a ReachableCell from `pack`.
			 *
			 * @param pack source ObjectPack
			 * @return unserialized ReachableCell
			 */
			static model::ReachableCell from_datapack(
					const ObjectPack& pack);
		};

	/**
	 * Polymorphic GraphCellBase Serializer specialization.
	 *
	 * | Serialization Scheme ||
	 * |----------------------||
	 * | `Derived` ObjectPack serialization | ReachableCell ObjectPack serialization |
	 *
	 * The `Derived` part is serialized using the
	 * Serializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint. The input GraphCellBase
	 * pointer is dynamically cast to `Derived` or ReachableCell when required
	 * to call the proper Serializer specialization.
	 *
	 * @tparam GraphCellType final fpmas::model::GraphCellBase type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename GraphCellType, typename Derived>
		struct Serializer<PtrWrapper<fpmas::model::GraphCellBase<GraphCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::GraphCellBase<GraphCellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the GraphCellBase
			 * pointed to by `ptr` in `p`.
			 */
			static std::size_t size(const ObjectPack& p, const Ptr& ptr) {
				return p.size(PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get()))))
					+ p.size(static_cast<const fpmas::model::ReachableCell&>(*ptr));
			}

			/**
			 * Serializes the pointer to the polymorphic GraphCellBase into
			 * the specified ObjectPack.
			 *
			 * @param pack destination ObjectPack
			 * @param ptr pointer to a polymorphic SpatialAgentBase to serialize
			 */
			static void to_datapack(ObjectPack& pack, const Ptr& ptr) {
				// Derived serialization
				PtrWrapper<Derived> derived = PtrWrapper<Derived>(
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get())));
				const fpmas::model::ReachableCell& reachable_cell
					= static_cast<const fpmas::model::ReachableCell&>(*ptr);
				pack.put(derived);
				pack.put(reachable_cell);
			}

			/**
			 * Unserializes a polymorphic GraphCellBase from the specified
			 * ObjectPack.
			 *
			 * First, the `Derived` part, that extends `GraphCellBase` by
			 * definition, is unserialized using the
			 * `Serializer<fpmas::api::utils::PtrWrapper<Derived>`
			 * specialization. During this operation, a `Derived` instance is
			 * dynamically allocated, that might leave the `GraphCellBase`
			 * members undefined. The ReachableCell part is then unserialized
			 * into this instance, and the final `Derived` instance is returned
			 * in the form of a polymorphic `GraphCellBase` pointer.
			 *
			 * @param pack source ObjectPack
			 * @return unserialized pointer to a polymorphic `GraphCellBase`
			 */
			static Ptr from_datapack(const ObjectPack& pack) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				PtrWrapper<Derived> derived_ptr = pack
					.get<PtrWrapper<Derived>>();

				// ReachableCell unserialization into derived_ptr
				static_cast<fpmas::model::ReachableCell&>(*derived_ptr)
					= pack.get<fpmas::model::ReachableCell>();

				return derived_ptr.get();
			}
		};


	/**
	 * Polymorphic GraphCellBase LightSerializer specialization.
	 *
	 * The LightSerializer is directly call on the next `Derived` type: no
	 * data is added to / extracted from the current LightObjectPack.
	 *
	 * | Serialization Scheme |
	 * |----------------------|
	 * | `Derived` LightObjectPack serialization |
	 *
	 * The `Derived` part is serialized using the
	 * LightSerializer<PtrWrapper<Derived>> serialization, that can be defined
	 * externally without additional constraint (and potentially leaves the
	 * LightObjectPack empty).
	 *
	 * @tparam GraphCellType final fpmas::model::GraphCellBase type to serialize
	 * @tparam Derived next derived class in the polymorphic serialization
	 * chain
	 */
	template<typename GraphCellType, typename Derived>
		struct LightSerializer<PtrWrapper<fpmas::model::GraphCellBase<GraphCellType, Derived>>> {
			/**
			 * Pointer wrapper to a polymorphic SpatialAgentBase.
			 */
			typedef PtrWrapper<fpmas::model::GraphCellBase<GraphCellType, Derived>> Ptr;

			/**
			 * Returns the buffer size required to serialize the polymorphic
			 * GraphCellBase pointed to by `ptr` into `pack`, i.e. the buffer
			 * size required to serialize the `Derived` part of `ptr`.
			 */
			static std::size_t size(const LightObjectPack& pack, const Ptr& ptr) {
				return pack.size(PtrWrapper<Derived>(
							const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get()))
							));
			}

			/**
			 * Serializes the pointer to the polymorphic GraphCellBase into
			 * the specified LightObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%to_datapack()`,
			 * without adding any `GraphCellBase` specific data to the
			 * LightObjectPack.
			 *
			 * @param pack destination LightObjectPack
			 * @param ptr agent pointer to serialize 
			 */
			static void to_datapack(LightObjectPack& pack, const Ptr& ptr) {
				// Derived serialization
				LightSerializer<PtrWrapper<Derived>>::to_datapack(
						pack,
						const_cast<Derived*>(dynamic_cast<const Derived*>(ptr.get()))
						);
			}

			/**
			 * Unserializes a polymorphic GraphCellBase from the specified
			 * ObjectPack.
			 *
			 * Effectively calls
			 * `LightSerializer<fpmas::api::utils::PtrWrapper<Derived>>::%from_datapack()`,
			 * without extracting any `GraphCellBase` specific data from the
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
}}}
#endif
