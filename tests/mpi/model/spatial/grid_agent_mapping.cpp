#include "fpmas/model/spatial/grid_agent_mapping.h"
#include "fpmas/model/spatial/grid.h"
#include "fpmas.h"

#include "gmock/gmock.h"

using namespace testing;
using namespace fpmas::model;

class AgentMappingTest : public Test {
	protected:
		static constexpr DiscreteCoordinate grid_width {100};
		static constexpr DiscreteCoordinate grid_height {100};
		static const std::size_t num_agent;

		std::array<fpmas::model::GridCell, grid_width*grid_height> cells;

		AgentMappingTest() {
			fpmas::seed(fpmas::default_seed);
		}

		void SetUp() override {
			for(DiscreteCoordinate x = 0; x < grid_width; x++)
				for(DiscreteCoordinate y = 0; y < grid_height; y++)
					cells[grid_width*x+y] = {{x, y}};
		}

		void check_local_mapping(
				fpmas::api::model::SpatialAgentMapping<fpmas::api::model::GridCell>& mapping
				);
};
const std::size_t AgentMappingTest::num_agent = 4000;

void AgentMappingTest::check_local_mapping(
				fpmas::api::model::SpatialAgentMapping<fpmas::api::model::GridCell>& mapping
				) {
	std::array<std::size_t, grid_width*grid_height> counts;
	for(std::size_t i = 0; i < counts.size() ; i++)
		counts[i] = mapping.countAt(&cells[i]);
	std::size_t total_count = std::accumulate(counts.begin(), counts.end(), 0);
	// First, the mapping must generate exactly 30 agent on the 10x10 grid
	ASSERT_EQ(total_count, num_agent);

	fpmas::communication::TypedMpi<std::array<std::size_t, counts.size()>> mpi
		(fpmas::communication::WORLD);
	std::array<std::size_t, counts.size()> proc_0_counts = mpi.bcast(counts, 0);

	// The mapping is generated intependently on each process.
	// However, it spite of randomness, **all processes** are required to
	// generate **exactly** the same mapping.
	ASSERT_THAT(counts, ElementsAreArray(proc_0_counts));
}


class UniformGridAgentMappingTest : public AgentMappingTest {
	protected:
		fpmas::model::UniformGridAgentMapping mapping {grid_width, grid_height, num_agent};
};

TEST_F(UniformGridAgentMappingTest, consistency_across_processes) {
	check_local_mapping(mapping);
}

class ConstrainedGridAgentMappingTest : public AgentMappingTest {
	protected:
		fpmas::model::ConstrainedGridAgentMapping mapping {
			grid_width, grid_height, num_agent, 2
		};
};

TEST_F(ConstrainedGridAgentMappingTest, consistency_across_processes) {
	check_local_mapping(mapping);
}
