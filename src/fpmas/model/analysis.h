#ifndef FPMAS_MODEL_ANALYSIS_H
#define FPMAS_MODEL_ANALYSIS_H

#include "fpmas/api/model/model.h"

/** \file src/fpmas/model/analysis.h
 * Contains utilies to perform model analysis.
 */

namespace fpmas { namespace model {

	/**
	 * Returns a vector containing the ids of \LOCAL agents contained in the
	 * specified group.
	 *
	 * @param group \AgentGroup to consider
	 * @return ids of \LOCAL agents 
	 */
	std::vector<api::graph::DistributedId> local_agent_ids(
			const api::model::AgentGroup& group);

}}
#endif
