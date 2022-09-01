#include "analysis.h"

namespace fpmas { namespace model {
	std::vector<api::graph::DistributedId> local_agent_ids(
			const api::model::AgentGroup &group) {
		auto local_agents = group.localAgents();
		std::vector<api::graph::DistributedId> ids(local_agents.size());
		auto it = ids.begin();
		for(auto& agent : local_agents) {
			*it = agent->node()->getId();
			++it;
		}
		return ids;
	}
}}
