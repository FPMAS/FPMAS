#include "fpmas/model/spatial/graph_range.h"
#include "fpmas/synchro/ghost/ghost_mode.h"

#include "../test_agents.h"

using namespace testing;


class TestGraphRange : public Test {
	protected:
		fpmas::model::SpatialModel<fpmas::synchro::GhostMode, fpmas::model::GraphCell> model;
		std::unordered_map<int, fpmas::graph::DistributedId> index_to_cell;
		std::unordered_map<fpmas::graph::DistributedId, int> cell_to_index;
		int cell_count;

		void SetUp() override {
			cell_count = 2*model.getMpiCommunicator().getSize();
			fpmas::graph::PartitionMap partition;

			FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
				std::vector<fpmas::model::GraphCell*> cells;
				for(int i = 0; i < cell_count; i++) {
					auto cell = new fpmas::model::GraphCell;
					model.add(cell);
					cells.push_back(cell);
					partition[cell->node()->getId()] = i / 2;
					index_to_cell[i] = cell->node()->getId();
					cell_to_index[cell->node()->getId()] = i;
				}
				for(std::size_t i = 0; i < cells.size(); i++) {
					auto n1 = cells[i];
					auto n2 = cells[(i+1)%cells.size()];
					model.link(n1, n2, fpmas::api::model::CELL_SUCCESSOR);
					model.link(n2, n1, fpmas::api::model::CELL_SUCCESSOR);
				}
			}
			fpmas::model::GraphRange<>::synchronize(model);
			model.graph().distribute(partition);

			fpmas::communication::TypedMpi<decltype(index_to_cell)> index_to_cell_mpi(
					model.getMpiCommunicator()
					);
			index_to_cell = index_to_cell_mpi.bcast(index_to_cell, 0);

			fpmas::communication::TypedMpi<decltype(cell_to_index)> cell_to_index_mpi(
					model.getMpiCommunicator()
					);
			cell_to_index = cell_to_index_mpi.bcast(cell_to_index, 0);

			for(auto item : cell_to_index) {
				if(model.graph().getNodes().count(item.first) == 0) {
					auto temp_cell = new fpmas::model::GraphCell;
					temp_cell->addGroupId(model.cellGroup().groupId());
					auto temp_node = new fpmas::graph::DistributedNode<fpmas::model::AgentPtr>(item.first, {temp_cell});
					temp_node->setLocation(item.second/2);
					model.graph().insertDistant(temp_node);
				}
			}
			model.graph().synchronizationMode().getDataSync().synchronize();

		}

};

TEST_F(TestGraphRange, test_range) {
	fpmas::model::GraphRange<> range(fpmas::model::INCLUDE_LOCATION);

	for(auto agent : model.cellGroup().agents()) {
		std::vector<DistributedId> agents_in_range;
		for(auto other_agent : model.cellGroup().agents()) {
			if(range.contains(
						dynamic_cast<fpmas::model::GraphCell*>(agent),
						dynamic_cast<fpmas::model::GraphCell*>(other_agent))
			  )
				agents_in_range.push_back(other_agent->node()->getId());
		}
		ASSERT_THAT(agents_in_range, UnorderedElementsAre(
					index_to_cell[(cell_to_index[agent->node()->getId()])],
					index_to_cell[(cell_to_index[agent->node()->getId()]+1)%cell_count],
					index_to_cell[(cell_count+cell_to_index[agent->node()->getId()]-1)%cell_count]
					));
	}
}
