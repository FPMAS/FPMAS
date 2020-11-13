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

	class GridCellBase : public virtual api::model::GridCell {
		private:
			DiscretePoint _location;

		public:
			GridCellBase(DiscretePoint location)
				: _location(location) {}

			DiscretePoint location() override {
				return _location;
			}
	};

	class GridCell : public GridCellBase, public Cell<GridCell> {
		public:
			GridCell(DiscretePoint location)
				: GridCellBase(location) {}

	};

	template<typename AgentType>
	class GridAgent :
		public api::model::GridAgent,
		public SpatialAgentBase<AgentType, fpmas::api::model::GridCell, GridAgent<AgentType>> {
			friend nlohmann::adl_serializer<fpmas::api::utils::PtrWrapper<GridAgent<AgentType>>>;

			private:
			DiscretePoint current_location;

			public:
			void moveToCell(fpmas::api::model::GridCell* cell) override;
			void moveToPoint(DiscretePoint point) override;
			DiscretePoint currentLocation() override {return current_location;}
		};

	template<typename AgentType>
		void GridAgent<AgentType>::moveToCell(fpmas::api::model::GridCell* cell) {
			this->updateLocation(cell);
			current_location = cell->location();
		}

	template<typename AgentType>
		void GridAgent<AgentType>::moveToPoint(DiscretePoint point) {
		}

	class VonNeumannNeighborhood : public api::model::EnvironmentBuilder<api::model::GridCell> {

	};
}}

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2);
}}}

namespace nlohmann {
	template<typename AgentType>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType>>> {
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::GridAgent<AgentType>> Ptr;
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<AgentType>(
						const_cast<AgentType*>(static_cast<const AgentType*>(ptr.get())));
				// Current base serialization
				j[1] = ptr->current_location;
			}

			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<AgentType> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<AgentType>>();

				// Initializes the current base
				derived_ptr->current_location = j[1].get<fpmas::api::model::DiscretePoint>();
				return derived_ptr.get();
			}
		};

}
#endif
