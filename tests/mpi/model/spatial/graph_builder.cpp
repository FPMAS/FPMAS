#include "fpmas/model/spatial/graph_builder.h"
#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/communication/communication.h"
#include "fpmas/model/spatial/graph.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "fpmas/synchro/ghost/ghost_mode.h"

#include "gmock/gmock.h"
#include <gmock/gmock-cardinalities.h>
#include <gmock/gmock-spec-builders.h>

using namespace testing;

class MockGraphBuilder : public fpmas::api::graph::DistributedGraphBuilder<fpmas::model::AgentPtr> {
	public:
	MOCK_METHOD(std::vector<fpmas::api::model::AgentNode*>, build, (
				fpmas::api::graph::DistributedNodeBuilder<fpmas::model::AgentPtr>&,
				fpmas::api::graph::LayerId,
				fpmas::api::model::AgentGraph&), (override));
};

struct MockGraphCellAllocator {
	MOCK_METHOD(fpmas::model::GraphCell*, call, (), ());
};

TEST(CellNetworkBuilder, build) {
	MockGraphBuilder mock_graph_builder;
	EXPECT_CALL(mock_graph_builder, build(_, fpmas::api::model::CELL_SUCCESSOR, _))
		.WillOnce(Invoke([] (
						fpmas::api::graph::DistributedNodeBuilder<fpmas::model::AgentPtr>& node_builder,
						fpmas::api::graph::LayerId,
						fpmas::api::model::AgentGraph& graph) {
				std::vector<fpmas::api::model::AgentNode*> nodes;
				while(node_builder.nodeCount() > 0)
					nodes.push_back(node_builder.buildNode(graph));

				// Builds a fake distant node (assumes at least one node is
				// built on each process)
				fpmas::communication::TypedMpi<fpmas::graph::DistributedId> id_mpi(
						graph.getMpiCommunicator());
				int rank = graph.getMpiCommunicator().getRank();
				int size = graph.getMpiCommunicator().getSize();
				int next_rank = (rank+1)%size;
				int prev_rank = (rank-1)%size;

				fpmas::communication::Request req;
				id_mpi.Isend(nodes[0]->getId(), next_rank, 0, req);
				fpmas::graph::DistributedId id = id_mpi.recv(prev_rank, 0);
				graph.getMpiCommunicator().wait(req);

				node_builder.buildDistantNode(id, next_rank, graph);
				return nodes;
					}));

	std::size_t allocator_counter = 0;
	MockGraphCellAllocator mock_graph_cell_allocator;
	EXPECT_CALL(mock_graph_cell_allocator, call)
		.Times(AnyNumber())
		.WillRepeatedly(Invoke(
					[&allocator_counter] {
						allocator_counter++;
						return new fpmas::model::GraphCell;
						}
					));

	std::size_t distant_allocator_counter = 0;
	MockGraphCellAllocator mock_distant_graph_cell_allocator;
	EXPECT_CALL(mock_distant_graph_cell_allocator, call)
		.Times(AnyNumber())
		.WillRepeatedly(Invoke(
					[&distant_allocator_counter] {
						distant_allocator_counter++;
						return new fpmas::model::GraphCell;
						}
					));


	std::function<fpmas::model::GraphCell*()> cell_allocator([&mock_graph_cell_allocator] {
			return mock_graph_cell_allocator.call();
			});
	std::function<fpmas::model::GraphCell*()> distant_cell_allocator([&mock_distant_graph_cell_allocator] {
			return mock_distant_graph_cell_allocator.call();
			});

	const std::size_t expected_num_cell = 4*fpmas::communication::WORLD.getSize();
	fpmas::model::CellNetworkBuilder<fpmas::model::GraphCell> cell_network_builder(
			mock_graph_builder, expected_num_cell,
			cell_allocator, distant_cell_allocator
			);

	fpmas::model::SpatialModel<fpmas::synchro::GhostMode, fpmas::model::GraphCell> model;
	auto& group1 = model.buildGroup(1);
	auto& group2 = model.buildGroup(2);
	cell_network_builder.build(model, {group1, group2});

	fpmas::communication::TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	std::size_t num_cell = fpmas::communication::all_reduce(int_mpi, model.cells().size());
	for(auto cell : model.cells()) {
		ASSERT_THAT(cell->groups(), Contains(Pointee(Property(&fpmas::api::model::AgentGroup::groupId, 1))));
		ASSERT_THAT(cell->groups(), Contains(Pointee(Property(&fpmas::api::model::AgentGroup::groupId, 2))));
	}

	ASSERT_EQ(num_cell, expected_num_cell);

	std::size_t num_alloc = fpmas::communication::all_reduce(int_mpi, allocator_counter);
	ASSERT_EQ(num_alloc, expected_num_cell);

	std::size_t num_distant_alloc
		= fpmas::communication::all_reduce(int_mpi, distant_allocator_counter);
	ASSERT_EQ(num_distant_alloc, model.getMpiCommunicator().getSize());
}

TEST(CellNetworkBuilder, small_world_graph_builder_integration) {
	std::size_t expected_num_cell = 4*fpmas::communication::WORLD.getSize();
	fpmas::model::SmallWorldGraphBuilder graph_builder(0.1, 4);
	fpmas::model::CellNetworkBuilder<fpmas::model::GraphCell> model_builder(
			graph_builder, expected_num_cell
			);

	fpmas::model::SpatialModel<fpmas::synchro::GhostMode, fpmas::model::GraphCell> model;
	auto& group1 = model.buildGroup(1);
	auto& group2 = model.buildGroup(2);
	model_builder.build(model, {group1, group2});

	fpmas::communication::TypedMpi<std::size_t> int_mpi(model.getMpiCommunicator());
	std::size_t num_cell = fpmas::communication::all_reduce(int_mpi, model.cells().size());
	for(auto cell : model.cells()) {
		ASSERT_THAT(cell->groups(), Contains(Pointee(Property(&fpmas::api::model::AgentGroup::groupId, 1))));
		ASSERT_THAT(cell->groups(), Contains(Pointee(Property(&fpmas::api::model::AgentGroup::groupId, 2))));
	}

	ASSERT_EQ(num_cell, expected_num_cell);

}
