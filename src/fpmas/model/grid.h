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
		public SpatialAgentBase<AgentType, fpmas::api::model::GridCell> {

		private:
			DiscretePoint current_location;

		public:
			void moveToCell(fpmas::api::model::GridCell* cell) override;
			void moveToPoint(DiscretePoint point) override;
			DiscretePoint currentLocation() override {return current_location;}

			static GridAgent* from_json(const nlohmann::json& j) {
				fpmas::api::utils::PtrWrapper<AgentType> derived_ptr;
				if(j.contains("derived"))
					derived_ptr = j.at("derived").get<fpmas::api::utils::PtrWrapper<AgentType>>();
				else
					derived_ptr = nlohmann::json().get<fpmas::api::utils::PtrWrapper<AgentType>>();

				derived_ptr->current_location = j.at("base").get<DiscretePoint>();
				return derived_ptr;
			}

			static void to_json(nlohmann::json& j, const GridAgent* agent) {
				nlohmann::json derived = fpmas::api::utils::PtrWrapper<const AgentType>(static_cast<const AgentType*>(agent));
				if(!derived.is_null())
					j["derived"] = derived;
				j["base"] = agent->current_location;
			}

			static void to_json(nlohmann::json& j, const AgentType* agent) {
			}
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
#endif
