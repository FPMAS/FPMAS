#include "fpmas/model/serializer.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "../mocks/model/mock_environment.h"
#include "test_agents.h"
#include "fpmas/api/graph/distributed_id.h"

using namespace testing;

class EnvironmentTest : public ::testing::Test {
	public:
		fpmas::model::Model<fpmas::synchro::GhostMode> model;
		fpmas::model::Environment environment {model};
		fpmas::model::AgentGroup& agent_group {model.buildGroup(0, TestLocatedAgent::behavior)};
		std::unordered_map<fpmas::api::graph::DistributedId, int> agent_id_to_index;
		std::unordered_map<int, fpmas::api::graph::DistributedId> index_to_agent_id;

		fpmas::scheduler::detail::LambdaTask check_model_task {[&]() -> void {this->checkModelState();}};
		fpmas::scheduler::Job check_model_job {check_model_task};

		void SetUp() override {
			fpmas::graph::PartitionMap partition;
			FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
				std::vector<fpmas::api::model::Cell*> cells;
				std::vector<fpmas::api::model::LocatedAgent*> agents;
				for(int i = 0; i < model.getMpiCommunicator().getSize(); i++) {
					auto cell = new TestCell(i);
					auto agent = new TestLocatedAgent;
					cells.push_back(cell);
					agents.push_back(agent);

					agent_group.add(agent);
					agent_id_to_index[agent->node()->getId()] = i;
					index_to_agent_id[i] = agent->node()->getId();
				}

				environment.add(cells);
				for(std::size_t i = 0; i < cells.size(); i++) {
					agents[i]->moveToCell(cells[i]);
					partition[agents[i]->node()->getId()] = i;
					partition[cells[i]->node()->getId()] = i;
				}

				for(std::size_t i = 1; i < cells.size() - 1; i++) {
					model.link(cells[i], cells[i-1], fpmas::api::model::NEIGHBOR_CELL);
					model.link(cells[i], cells[i+1], fpmas::api::model::NEIGHBOR_CELL);
				}
				if(cells.size() > 1) {
					if(cells.size() == 2) {
						model.link(cells[0], cells[1], fpmas::api::model::NEIGHBOR_CELL);
						model.link(cells[1], cells[0], fpmas::api::model::NEIGHBOR_CELL);
					} else {
						model.link(cells[0], cells[1], fpmas::api::model::NEIGHBOR_CELL);
						model.link(cells[0], cells[cells.size()-1], fpmas::api::model::NEIGHBOR_CELL);
						model.link(cells[cells.size()-1], cells[cells.size()-2], fpmas::api::model::NEIGHBOR_CELL);
						model.link(cells[cells.size()-1], cells[0], fpmas::api::model::NEIGHBOR_CELL);
					}
				}
			}
			model.graph().distribute(partition);

			fpmas::communication::TypedMpi<decltype(agent_id_to_index)> id_to_index_mpi(model.getMpiCommunicator());
			fpmas::communication::TypedMpi<decltype(index_to_agent_id)> index_to_id_mpi(model.getMpiCommunicator());

			agent_id_to_index = id_to_index_mpi.bcast(agent_id_to_index, 0);
			index_to_agent_id = index_to_id_mpi.bcast(index_to_agent_id, 0);
		}

		void checkModelState() {
			int comm_size = model.getMpiCommunicator().getSize();

			//auto local_cell = environment.localCells().at(0);
			for(auto cell : environment.localCells()) {
				if(comm_size > 1)
					if(comm_size == 2)
						ASSERT_THAT(cell->neighborhood(), SizeIs(1));
					else
						ASSERT_THAT(cell->neighborhood(), SizeIs(2));
				else
					ASSERT_THAT(cell->neighborhood(), SizeIs(0));
			}
			for(auto agent : agent_group.localAgents()) {
				ASSERT_THAT(
						agent->node()->outNeighbors(fpmas::api::model::LOCATION), 
						SizeIs(1));

				auto location = static_cast<TestCell*>(dynamic_cast<fpmas::api::model::LocatedAgent*>(agent)->location());

				float step_f;
				std::modf(model.runtime().currentDate(), &step_f);
				fpmas::api::scheduler::TimeStep step = step_f;
				ASSERT_EQ(location->index, (agent_id_to_index[agent->node()->getId()] + step) % comm_size);

				std::vector<int> expected_move_index {location->index};
				if(comm_size > 1) {
					if(comm_size == 2) {
						expected_move_index.push_back((location->index+1) % comm_size);
					} else {
						expected_move_index.push_back((location->index+1) % comm_size);
						expected_move_index.push_back((comm_size + location->index-1) % comm_size);
					}
				}

				std::vector<int> actual_move_index;
				for(auto cell : dynamic_cast<fpmas::model::AgentBase<TestLocatedAgent>*>(agent)->outNeighbors<TestCell>(fpmas::api::model::MOVE)) {
					actual_move_index.push_back(cell->index);
				}

				ASSERT_THAT(actual_move_index, UnorderedElementsAreArray(expected_move_index));

				std::vector<int> expected_perceptions_index;
				if(comm_size > 1) {
					if(comm_size == 2) {
						expected_perceptions_index.push_back((location->index+1) % comm_size);
					} else {
						expected_perceptions_index.push_back((location->index+1) % comm_size);
						expected_perceptions_index.push_back((comm_size + location->index-1) % comm_size);
					}
				}
				std::vector<int> actual_perceptions_index;
				for(auto perception : agent->node()->outNeighbors(fpmas::api::model::PERCEPTION))
					actual_perceptions_index.push_back((agent_id_to_index[perception->getId()] + step) % comm_size);

				ASSERT_THAT(actual_perceptions_index, UnorderedElementsAreArray(expected_perceptions_index));

				ASSERT_THAT(agent->node()->outNeighbors(fpmas::api::model::NEW_LOCATION), IsEmpty());
				ASSERT_THAT(agent->node()->outNeighbors(fpmas::api::model::NEW_MOVE), IsEmpty());
				ASSERT_THAT(agent->node()->outNeighbors(fpmas::api::model::NEW_PERCEIVE), IsEmpty());
			}
		}
};

TEST_F(EnvironmentTest, init_location_test) {
	model.scheduler().schedule(0, environment.initLocationAlgorithm(1, 1));
	model.runtime().run(1);

	checkModelState();
}

TEST_F(EnvironmentTest, distributed_move_algorithm) {
	model.scheduler().schedule(0, environment.initLocationAlgorithm(1, 1));
	model.scheduler().schedule(1, 1, environment.distributedMoveAlgorithm(agent_group, 1, 1));
	model.scheduler().schedule(0.1, 1, check_model_job);

	model.runtime().run(3);
}
