#ifndef FPMAS_GRID_H
#define FPMAS_GRID_H

#include "fpmas/api/model/grid.h"
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
					fpmas::api::model::Range<api::model::GridCell>& mobility_range,
					fpmas::api::model::Range<api::model::GridCell>& perception_range)
				: model::SpatialAgentBase<AgentType, api::model::GridCell, GridAgent<AgentType, Derived>>(
						mobility_range, perception_range) {}

			using model::SpatialAgentBase<AgentType, api::model::GridCell, GridAgent<AgentType, Derived>>::moveTo;
			void moveTo(api::model::Cell* cell) override;
			void moveTo(DiscretePoint point) override;

			public:
			DiscretePoint locationPoint() const override {return current_location_point;}
		};

	template<typename AgentType, typename Derived>
		void GridAgent<AgentType, Derived>::moveTo(api::model::Cell* cell) {
			this->updateLocation(cell);
			if(auto grid_cell = dynamic_cast<api::model::GridCell*>(cell))
				current_location_point = grid_cell->location();
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

	class VonNeumannGridBuilder : public api::model::SpatialModelBuilder<api::model::GridCell> {
		private:
			typedef std::vector<std::vector<api::model::GridCell*>> CellMatrix;
			void allocate(CellMatrix& matrix, DiscreteCoordinate width, DiscreteCoordinate height);

			api::model::GridCellFactory& cell_factory;
			DiscreteCoordinate width;
			DiscreteCoordinate height;

			CellMatrix buildLocalGrid(
					api::model::SpatialModel& model,
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
					api::model::SpatialModel& spatial_model);
	};

	class RandomGridAgentMapping : public api::model::SpatialAgentMapping<api::model::GridCell> {
		private:
			std::unordered_map<DiscreteCoordinate, std::unordered_map<DiscreteCoordinate, std::size_t>> count_map;
		public:
			RandomGridAgentMapping(
					api::random::Distribution<DiscreteCoordinate>&& x,
					api::random::Distribution<DiscreteCoordinate>&& y,
					std::size_t agent_count);

			RandomGridAgentMapping(
					api::random::Distribution<DiscreteCoordinate>& x,
					api::random::Distribution<DiscreteCoordinate>& y,
					std::size_t agent_count);

			std::size_t countAt(api::model::GridCell* cell) override {
				// When values are not in the map, 0 (default initialized) will
				// be returned
				return count_map[cell->location().x][cell->location().y];
			}
	};

	class UniformGridAgentMapping : public RandomGridAgentMapping {
		public:
			UniformGridAgentMapping(
					DiscreteCoordinate grid_width,
					DiscreteCoordinate grid_height,
					std::size_t agent_count) 
				: RandomGridAgentMapping(
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_width-1),
						random::UniformIntDistribution<DiscreteCoordinate>(0, grid_height-1),
						agent_count) {}
	};
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
#endif
