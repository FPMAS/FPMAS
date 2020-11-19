#ifndef FPMAS_GRID_H
#define FPMAS_GRID_H

#include "fpmas/api/model/grid.h"
#include "environment.h"

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
			auto mobility_field = this->template outNeighbors<fpmas::api::model::GridCell>(EnvironmentLayers::MOVE);
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

	class VonNeumannNeighborhood : public api::model::EnvironmentBuilder {
		private:
			typedef std::vector<std::vector<api::model::GridCell*>> CellMatrix;
			void allocate(CellMatrix& matrix, DiscreteCoordinate width, DiscreteCoordinate height);

			api::model::GridCellFactory& cell_factory;
			DiscreteCoordinate width;
			DiscreteCoordinate height;

			CellMatrix buildLocalGrid(
					api::model::Model& model,
					api::model::Environment& environment,
					DiscreteCoordinate min_x, DiscreteCoordinate max_x,
					DiscreteCoordinate min_y, DiscreteCoordinate max_y);

		public:
			VonNeumannNeighborhood(
					api::model::GridCellFactory& cell_factory,
					DiscreteCoordinate width,
					DiscreteCoordinate height)
				: cell_factory(cell_factory), width(width), height(height) {}

			void build(
					api::model::Model& model,
					api::model::Environment& environment);
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
