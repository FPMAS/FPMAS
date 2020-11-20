#include "fpmas/model/serializer.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "../mocks/model/mock_environment.h"
#include "../mocks/runtime/mock_runtime.h"
#include "../mocks/scheduler/mock_scheduler.h"
#include "test_agents.h"
#include "fpmas/api/graph/distributed_id.h"
#include "fpmas/random/random.h"

using namespace testing;

template<template<typename> class SYNC_MODE>
class EnvironmentTestBase : public ::testing::Test {
	protected:
		unsigned int range_size;
		fpmas::model::Model<SYNC_MODE> model;
		fpmas::model::Environment environment;
		fpmas::model::AgentGroup& agent_group {model.buildGroup(0, TestSpatialAgent::behavior)};
		std::unordered_map<fpmas::api::graph::DistributedId, int> agent_id_to_index;
		std::unordered_map<int, fpmas::api::graph::DistributedId> index_to_agent_id;

		int num_cells_in_ring {model.getMpiCommunicator().getSize()};

		fpmas::scheduler::detail::LambdaTask check_model_task {[&]() -> void {this->checkModelState();}};
		fpmas::scheduler::Job check_model_job {check_model_task};

		EnvironmentTestBase(int range_size)
			: range_size(range_size), environment(model, range_size, range_size) {}

		void SetUp() override {
			fpmas::graph::PartitionMap partition;
			FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
				std::vector<TestCell*> cells;
				std::vector<fpmas::api::model::SpatialAgent*> agents;
				for(int i = 0; i < model.getMpiCommunicator().getSize(); i++) {
					auto cell = new TestCell(i);
					auto agent = new TestSpatialAgent(range_size, num_cells_in_ring);
					cells.push_back(cell);
					agents.push_back(agent);

					agent_group.add(agent);
					agent_id_to_index[agent->node()->getId()] = i;
					index_to_agent_id[i] = agent->node()->getId();

					environment.add(cell);
					environment.add(agent);

					agent->initLocation(cells[i]);
					partition[agent->node()->getId()] = i;
					partition[cell->node()->getId()] = i;
				}

				int comm_size = model.getMpiCommunicator().getSize();
				if(comm_size > 1) {
					if(comm_size > 2) {
						for(std::size_t i = 0; i < cells.size(); i++) {
							model.link(cells[i % comm_size], cells[(comm_size+i-1)%comm_size], fpmas::api::model::NEIGHBOR_CELL);
							model.link(cells[i % comm_size], cells[(i+1) % comm_size], fpmas::api::model::NEIGHBOR_CELL);
						}
					} else {
						model.link(cells[0], cells[1], fpmas::api::model::NEIGHBOR_CELL);
						model.link(cells[1], cells[0], fpmas::api::model::NEIGHBOR_CELL);
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

			for(auto cell : environment.cells()) {
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

				auto location = dynamic_cast<TestSpatialAgent*>(agent)->testLocation();

				float step_f;
				std::modf(model.runtime().currentDate(), &step_f);
				fpmas::api::scheduler::TimeStep step = step_f;
				ASSERT_EQ(location->index, (agent_id_to_index[agent->node()->getId()] + step) % comm_size);

				std::set<int> expected_move_index {location->index};
				if(comm_size > 1) {
					if(comm_size == 2) {
						expected_move_index.insert((location->index+1) % comm_size);
					} else {
						for(unsigned int i = 1; i <= range_size; i++) {
							expected_move_index.insert((location->index+i) % comm_size);
							expected_move_index.insert((comm_size + location->index-i) % comm_size);
						}
					}
				}

				std::vector<int> actual_move_index;
				for(auto cell : dynamic_cast<TestSpatialAgent*>(agent)->outNeighbors<TestCell>(fpmas::api::model::MOVE)) {
					actual_move_index.push_back(cell->index);
				}

				ASSERT_THAT(actual_move_index, UnorderedElementsAreArray(expected_move_index));

				std::set<int> expected_perceptions_index;
				if(comm_size > 1) {
					if(comm_size == 2) {
						expected_perceptions_index.insert((location->index+1) % comm_size);
					} else {
						for(unsigned int i = 1; i <= range_size; i++) {
							expected_perceptions_index.insert((location->index+i) % comm_size);
							expected_perceptions_index.insert((comm_size + location->index-i) % comm_size);
						}
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

		void testInit() {
			model.scheduler().schedule(0, environment.initLocationAlgorithm());
			model.runtime().run(1);

			checkModelState();
		}

		void testDistributedMoveAlgorithm() {
			model.scheduler().schedule(0, environment.initLocationAlgorithm());
			model.scheduler().schedule(1, 1, environment.distributedMoveAlgorithm(agent_group));
			model.scheduler().schedule(0.1, 1, check_model_job);

			model.runtime().run(3 * model.getMpiCommunicator().getSize());
		}
};

typedef EnvironmentTestBase<fpmas::synchro::GhostMode> EnvironmentTest_GhostMode;
typedef EnvironmentTestBase<fpmas::synchro::HardSyncMode> EnvironmentTest_HardSyncMode;

class EnvironmentTest_GhostMode_SimpleRange : public EnvironmentTest_GhostMode {
	protected:
		EnvironmentTest_GhostMode_SimpleRange()
			: EnvironmentTest_GhostMode(1) {}
};

TEST_F(EnvironmentTest_GhostMode_SimpleRange, init_location_test) {
	testInit();
}

TEST_F(EnvironmentTest_GhostMode_SimpleRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

class EnvironmentTest_HardSyncMode_SimpleRange : public EnvironmentTest_HardSyncMode {
	protected:
		EnvironmentTest_HardSyncMode_SimpleRange()
			: EnvironmentTest_HardSyncMode(1) {}
};

TEST_F(EnvironmentTest_HardSyncMode_SimpleRange, init_location_test) {
	testInit();
}

TEST_F(EnvironmentTest_HardSyncMode_SimpleRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

class EnvironmentTest_GhostMode_ComplexRange : public EnvironmentTest_GhostMode {
	protected:
		EnvironmentTest_GhostMode_ComplexRange()
			: EnvironmentTest_GhostMode(2) {}
};

TEST_F(EnvironmentTest_GhostMode_ComplexRange, init_location_test) {
	testInit();
}

TEST_F(EnvironmentTest_GhostMode_ComplexRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

class EnvironmentTest_HardSyncMode_ComplexRange : public EnvironmentTest_HardSyncMode {
	protected:
		EnvironmentTest_HardSyncMode_ComplexRange()
			: EnvironmentTest_HardSyncMode(2) {}
};

TEST_F(EnvironmentTest_HardSyncMode_ComplexRange, init_location_test) {
	testInit();
}

TEST_F(EnvironmentTest_HardSyncMode_ComplexRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

class SpatialAgentBuilderTest : public Test {
/*
 *    class FakeSpatialAgent
 *        : public fpmas::model::SpatialAgent<FakeSpatialAgent, MockCell> {
 *            static const MockRange<MockCell> range;
 *            public:
 *            FakeSpatialAgent()
 *                : SpatialAgent(range, range) {}
 *
 *        };
 */

	protected:
	fpmas::random::DistributedGenerator<> rd;
	fpmas::random::UniformIntDistribution<> rand_int {0, 10};
	int num_cells {rand_int(rd)};

	std::vector<fpmas::api::model::Agent*> agents;
	std::vector<fpmas::api::model::Cell*> local_cells;
	MockJob mock_job;
	fpmas::api::scheduler::JobList fake_job_list {mock_job};
	NiceMock<MockEnvironment> environment;
	NiceMock<MockModel> model;

	MockRuntime mock_runtime;
	MockAgentGroup mock_group;
	StrictMock<MockSpatialAgentFactory> agent_factory;
	StrictMock<MockSpatialAgentMapping> agent_mapping;
	fpmas::model::SpatialAgentBuilder builder {model, environment};

	struct CountAgents {
		std::unordered_map<fpmas::api::model::Cell*, int> counts;

		void increase(fpmas::api::model::Cell* cell) {
			counts[cell]++;
		}
	};

	CountAgents count_agents;
	void SetUp() override {
		ON_CALL(model, runtime)
			.WillByDefault(ReturnRef(mock_runtime));
		ON_CALL(environment, initLocationAlgorithm)
			.WillByDefault(Return(fake_job_list));
		// Builds a random agent mapping on each process
		for(int i = 0; i < num_cells; i++) {
			int num_agents = rand_int(rd);
			local_cells.push_back(new MockCell);
			EXPECT_CALL(agent_mapping, countAt(local_cells.back()))
				.WillRepeatedly(Return(num_agents));

			for(int j = 0; j < num_agents; j++) {
				auto agent = new MockSpatialAgent;
				agents.push_back(agent);
				{
					InSequence s;
					EXPECT_CALL(mock_group, add(agent));
					EXPECT_CALL(*agent, initLocation)
						.WillOnce(Invoke(&count_agents, &CountAgents::increase));
				}
				// This is all the magic: using a common mapping, each process
				// only instantiate the agents located in local cells
				EXPECT_CALL(agent_factory, build)
					.WillOnce(Return(agent))
					.RetiresOnSaturation();
			}
		}
	}

	void TearDown() override {
		for(auto cell : local_cells)
			delete cell;
		for(auto agent : agents)
			delete agent;
	}
};

TEST_F(SpatialAgentBuilderTest, build) {
	// Expect initLocationAlgorithm execution
	EXPECT_CALL(mock_runtime, execute((Matcher<const fpmas::api::scheduler::JobList&>) ElementsAre(
					Property(&std::reference_wrapper<const fpmas::api::scheduler::Job>::get,Ref(mock_job)))));

	builder.build(mock_group, agent_factory, agent_mapping, local_cells);
}
