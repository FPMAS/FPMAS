#ifndef FPMAS_SPATIAL_MODEL_ANALYSIS_H
#define FPMAS_SPATIAL_MODEL_ANALYSIS_H

#include "fpmas/api/model/spatial/spatial_model.h"
#include "fpmas/model/analysis.h"

/** \file src/fpmas/model/spatial/analysis.h
 * Contains utilies to perform spatial model analysis.
 */

namespace fpmas { namespace model {

	/**
	 * Returns a vector containing the ids of \LOCAL \Cells in the specified
	 * model.
	 *
	 * @param model \SpatialModel to consider
	 * @return ids of \LOCAL \Cells
	 */
	template<typename CellType>
		std::vector<api::graph::DistributedId> local_cell_ids(
				const api::model::SpatialModel<CellType>& model) {
			return model::local_agent_ids(model.cellGroup());
		}
}}
#endif
