#ifndef FPMAS_SPATIAL_MODEL_ANALYSIS_H
#define FPMAS_SPATIAL_MODEL_ANALYSIS_H

#include "fpmas/api/model/spatial/spatial_model.h"
#include "fpmas/model/analysis.h"

namespace fpmas { namespace model {

	template<typename CellType>
		std::vector<api::graph::DistributedId> local_cell_ids(
				const api::model::SpatialModel<CellType>& model) {
			return model::local_agent_ids(model.cellGroup());
		}
}}
#endif
