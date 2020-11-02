#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "fpmas/api/model/environment.h"
#include "model.h"

namespace fpmas {
	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const api::model::EnvironmentLayers& layer);
	}}

	namespace model {
	using api::model::EnvironmentLayers;

	class CellBase : public api::model::Cell, public NeighborsAccess {
		protected:
			Neighbors<api::model::Cell> neighborCells() const;

			void updateLocation(api::model::Neighbor<api::model::LocatedAgent>& agent) override;
			void growMobilityRange(api::model::LocatedAgent* agent) override;
			void growPerceptionRange(api::model::LocatedAgent* agent) override;

		public:
			void updateRanges() override;
			void updatePerceptions() override;

	};

	//class GraphCell : public CellBase, public AgentBase<GraphCell> {

	//};
}}
#endif
