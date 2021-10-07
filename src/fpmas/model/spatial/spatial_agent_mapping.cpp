#include "spatial_agent_mapping.h"
#include "fpmas/utils/functional.h"

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

		fpmas::communication::TypedMpi<decltype(cell_ids)> mpi(comm);
		cell_ids = fpmas::communication::all_reduce(
				mpi, cell_ids, utils::Concat()
				);

		fpmas::random::UniformIntDistribution<std::size_t> dist(0, cell_ids.size()-1);
		for(std::size_t i = 0; i < agent_count; i++)
			mapping[cell_ids[dist(RandomMapping::rd)]]++;
	}

	std::size_t UniformAgentMapping::countAt(api::model::Cell* cell) {
		return mapping[cell->node()->getId()];
	}
}}
