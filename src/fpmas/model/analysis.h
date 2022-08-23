#ifndef FPMAS_MODEL_ANALYSIS_H
#define FPMAS_MODEL_ANALYSIS_H

#include "fpmas/api/model/model.h"

namespace fpmas { namespace model {

	std::vector<api::graph::DistributedId> local_agent_ids(
			api::model::AgentGroup& group);

}}
#endif
