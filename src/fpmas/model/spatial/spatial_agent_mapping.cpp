#include "spatial_agent_mapping.h"
#include "fpmas/utils/functional.h"
#include <fpmas/random/random.h>

namespace fpmas { namespace model {
	random::mt19937_64 RandomMapping::rd;

	void RandomMapping::seed(random::mt19937_64::result_type seed) {
		RandomMapping::rd.seed(seed);
	}

	UniformAgentMapping::UniformAgentMapping(
			api::communication::MpiCommunicator& comm,
			api::model::AgentGroup& cell_group,
			std::size_t agent_count
			) {
		// The same random generator is used on each process, so that the same
		// mapping is generated on any process
		std::vector<DistributedId> cell_ids;
		for(auto agent : cell_group.localAgents())
			cell_ids.push_back(agent->node()->getId());

		std::vector<DistributedId> local_cell_choices
			= fpmas::random::split_choices(comm, cell_ids, agent_count);

		for(auto& item : local_cell_choices)
			mapping[item]++;
	}

	std::size_t UniformAgentMapping::countAt(api::model::Cell* cell) {
		return mapping[cell->node()->getId()];
	}
}}
