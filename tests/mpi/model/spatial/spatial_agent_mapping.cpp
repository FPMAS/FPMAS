#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/model/spatial/spatial_agent_mapping.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "fpmas/model/spatial/graph.h"

#include "../../../mocks/model/mock_spatial_model.h"

using namespace testing;

class FakeSpatialAgent : public fpmas::model::SpatialAgent<FakeSpatialAgent, fpmas::model::GraphCell> {
	private:
		NiceMock<MockRange<fpmas::model::GraphCell>> range;

	public:
		FakeSpatialAgent() {}
		FakeSpatialAgent(const FakeSpatialAgent&)
			: FakeSpatialAgent() {}
		FakeSpatialAgent(FakeSpatialAgent&&)
			: FakeSpatialAgent() {}
		FakeSpatialAgent& operator=(const FakeSpatialAgent&) {return *this;}
		FakeSpatialAgent& operator=(FakeSpatialAgent&&) {return *this;}

		FPMAS_MOBILITY_RANGE(range);
		FPMAS_PERCEPTION_RANGE(range);
};

TEST(UniformAgentMappingTest, build) {
	fpmas::model::SpatialModel<fpmas::synchro::GhostMode, fpmas::model::GraphCell> model;

	fpmas::random::DistributedGenerator<> rd;
	// Edge count distribution: uniform between 1 and 3
	fpmas::random::UniformIntDistribution<std::size_t> edge_dist(1, 3);
	// Graph builder instance
	fpmas::model::DistributedUniformGraphBuilder builder(rd, edge_dist);

	// Node builder: used to dynamically instantiate GraphCells on each process
	fpmas::model::DistributedAgentNodeBuilder cell_builder(
			// Group to which Cells are added
			{model.cellGroup()},
			// Cell count
			5*model.getMpiCommunicator().getSize(),
			// Cell allocator. DefaultCells are used
			[] {return new fpmas::model::GraphCell;},
			// MPI communicator
			model.getMpiCommunicator()
			);
	// Builds the cell graph into the model.
	// Cells are linked on the CELL_SUCCESSOR layer.
	builder.build(cell_builder, fpmas::api::model::CELL_SUCCESSOR, model.graph());

	// Builds Agents using a `new Agent` statement
	fpmas::model::DefaultSpatialAgentFactory<FakeSpatialAgent> agent_factory;
	// Assigns each Agent to a random GraphCell
	fpmas::model::UniformAgentMapping mapping(
			model.getMpiCommunicator(), model.cellGroup(), 10
			);
	// SpatialAgentBuilder algorithm
	fpmas::model::SpatialAgentBuilder<fpmas::model::GraphCell> agent_builder;

	fpmas::model::IdleBehavior behavior;
	auto& group = model.buildMoveGroup(0, behavior);
	agent_builder.build(
			model, // SpatialModel
			{group}, // Groups to which each Agent must be added
			agent_factory, // Used to allocate Agents when required
			mapping // Specifies where Agents must be initialized
			);

	fpmas::communication::TypedMpi<std::size_t> mpi(model.getMpiCommunicator());

	std::size_t agent_count
		= fpmas::communication::all_reduce(mpi, group.localAgents().size());

	ASSERT_EQ(agent_count, 10);
}
