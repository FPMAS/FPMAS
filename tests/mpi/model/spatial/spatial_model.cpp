#include "fpmas/model/serializer.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "../mocks/model/mock_spatial_model.h"
#include "../mocks/runtime/mock_runtime.h"
#include "../mocks/scheduler/mock_scheduler.h"
#include "../test_agents.h"
#include "fpmas/api/graph/distributed_id.h"
#include "fpmas/random/random.h"

using namespace testing;

template<template<typename> class SYNC_MODE, int RangeSize>
class SpatialModelTestBase : public ::testing::Test {
	protected:
		unsigned int range_size = RangeSize;
		fpmas::model::SpatialModel<
			SYNC_MODE, TestCell,
			fpmas::model::StaticEndCondition<FakeRange, RangeSize, TestCell>
				> model;

		fpmas::model::MoveAgentGroup<TestCell>& agent_group {model.buildMoveGroup(0, TestSpatialAgent::behavior)};
		std::unordered_map<fpmas::api::graph::DistributedId, int> agent_id_to_index;
		std::unordered_map<int, fpmas::api::graph::DistributedId> index_to_agent_id;

		int num_cells_in_ring {model.getMpiCommunicator().getSize()};

		fpmas::scheduler::detail::LambdaTask check_model_task {
			[&]() -> void {this->checkModelState();}
		};
		fpmas::scheduler::Job check_model_job {check_model_task};

		SpatialModelTestBase() {}


		void SetUp() override {
			fpmas::graph::PartitionMap partition;
			FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
				std::vector<TestCell*> cells;
				std::vector<fpmas::api::model::SpatialAgent<TestCell>*> agents;
				for(int i = 0; i < model.getMpiCommunicator().getSize(); i++) {
					auto cell = new TestCell(i);
					auto agent = new TestSpatialAgent(range_size, num_cells_in_ring);
					cells.push_back(cell);
					agents.push_back(agent);

					agent_group.add(agent);
					agent_id_to_index[agent->node()->getId()] = i;
					index_to_agent_id[i] = agent->node()->getId();

					model.add(cell);

					agent->initLocation(cells[i]);
					partition[agent->node()->getId()] = i;
					partition[cell->node()->getId()] = i;
				}

				int comm_size = model.getMpiCommunicator().getSize();
				if(comm_size > 1) {
					if(comm_size > 2) {
						for(std::size_t i = 0; i < cells.size(); i++) {
							model.link(cells[i % comm_size], cells[(comm_size+i-1)%comm_size], fpmas::api::model::CELL_SUCCESSOR);
							model.link(cells[i % comm_size], cells[(i+1) % comm_size], fpmas::api::model::CELL_SUCCESSOR);
						}
					} else {
						model.link(cells[0], cells[1], fpmas::api::model::CELL_SUCCESSOR);
						model.link(cells[1], cells[0], fpmas::api::model::CELL_SUCCESSOR);
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

			for(auto cell : model.cells()) {
				if(comm_size > 1)
					if(comm_size == 2)
						ASSERT_THAT(cell->successors(), SizeIs(1));
					else
						ASSERT_THAT(cell->successors(), SizeIs(2));
				else
					ASSERT_THAT(cell->successors(), SizeIs(0));
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
			std::vector<fpmas::api::model::SpatialAgent<TestCell>*> agents;
			for(auto agent : agent_group.localAgents())
				agents.push_back(dynamic_cast<fpmas::api::model::SpatialAgent<TestCell>*>(agent));

			model.runtime().execute(agent_group.distributedMoveAlgorithm().jobs());

			checkModelState();
		}

		void testDistributedMoveAlgorithm() {
			// Init location
			std::vector<fpmas::api::model::SpatialAgent<TestCell>*> agents;
			for(auto agent : agent_group.localAgents())
				agents.push_back(dynamic_cast<fpmas::api::model::SpatialAgent<TestCell>*>(agent));

			model.runtime().execute(agent_group.distributedMoveAlgorithm().jobs());

			// Agent behavior. Since agent_group is a MoveAgentGroup, the
			// distributedMoveAlgorithm is automatically included.
			model.scheduler().schedule(1, 1, agent_group.jobs());
			model.scheduler().schedule(0.1, 1, check_model_job);

			model.runtime().run(3 * model.getMpiCommunicator().getSize());
		}
};

template<int MaxStep>
using SpatialModelTest_GhostMode = SpatialModelTestBase<fpmas::synchro::GhostMode, MaxStep>;

template<int MaxStep>
using SpatialModelTest_HardSyncMode = SpatialModelTestBase<fpmas::synchro::HardSyncMode, MaxStep>;

typedef SpatialModelTest_GhostMode<1> SpatialModelTest_GhostMode_SimpleRange;
typedef SpatialModelTest_GhostMode<2> SpatialModelTest_GhostMode_ComplexRange;
typedef SpatialModelTest_HardSyncMode<1> SpatialModelTest_HardSyncMode_SimpleRange;
typedef SpatialModelTest_HardSyncMode<2> SpatialModelTest_HardSyncMode_ComplexRange;

TEST_F(SpatialModelTest_GhostMode_SimpleRange, init_location_test) {
	testInit();
}

TEST_F(SpatialModelTest_GhostMode_SimpleRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

TEST_F(SpatialModelTest_HardSyncMode_SimpleRange, init_location_test) {
	testInit();
}

TEST_F(SpatialModelTest_HardSyncMode_SimpleRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

TEST_F(SpatialModelTest_GhostMode_ComplexRange, init_location_test) {
	testInit();
}

TEST_F(SpatialModelTest_GhostMode_ComplexRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

TEST_F(SpatialModelTest_HardSyncMode_ComplexRange, init_location_test) {
	testInit();
}

TEST_F(SpatialModelTest_HardSyncMode_ComplexRange, distributed_move_algorithm) {
	testDistributedMoveAlgorithm();
}

class SpatialAgentBuilderTest : public Test {
	protected:
	fpmas::random::DistributedGenerator<> rd;
	fpmas::random::UniformIntDistribution<> rand_int {5, 10};
	int num_cells {rand_int(rd)};

	std::vector<fpmas::api::model::Agent*> agents;
	std::vector<fpmas::api::model::Cell*> local_cells;
	MockJob mock_job;
	fpmas::api::scheduler::JobList fake_job_list {mock_job};
	NiceMock<MockDistributedMoveAlgorithm<fpmas::api::model::Cell>> mock_dist_move_algo;
	MockSpatialModel<fpmas::api::model::Cell, NiceMock> model;

	MockRuntime mock_runtime;
	std::array<MockAgentGroup, 2> mock_groups;
	NiceMock<MockMoveAgentGroup<fpmas::api::model::Cell>> mock_move_group;
	StrictMock<MockSpatialAgentFactory<fpmas::api::model::Cell>> agent_factory;
	StrictMock<MockSpatialAgentMapping<fpmas::api::model::Cell>> agent_mapping;
	fpmas::model::SpatialAgentBuilder<fpmas::api::model::Cell> builder;

	struct CountAgents {
		std::unordered_map<fpmas::api::model::Cell*, int> counts;

		void increase(fpmas::api::model::Cell* cell) {
			counts[cell]++;
		}
	};

	ExpectationSet add_to_temp_group;

	CountAgents count_agents;
	void SetUp() override {
		ON_CALL(model, runtime)
			.WillByDefault(ReturnRef(mock_runtime));
		ON_CALL(model, buildMoveGroup(-2, _))
			.WillByDefault(ReturnRef(mock_move_group));
		ON_CALL(mock_move_group, distributedMoveAlgorithm)
			.WillByDefault(ReturnRef(mock_dist_move_algo));
		ON_CALL(mock_dist_move_algo, jobs)
			.WillByDefault(Return(fake_job_list));
		// Builds a random agent mapping on each process
		for(int i = 0; i < num_cells; i++) {
			int num_agents = rand_int(rd);
			local_cells.push_back(new MockCell<>);
			EXPECT_CALL(agent_mapping, countAt(local_cells.back()))
				.WillRepeatedly(Return(num_agents));

			for(int j = 0; j < num_agents; j++) {
				auto agent = new MockSpatialAgent<fpmas::api::model::Cell>;
				agents.push_back(agent);

				add_to_temp_group += EXPECT_CALL(mock_move_group, add(agent));

				Sequence s1, s2;
				EXPECT_CALL(mock_groups[0], add(agent))
					.InSequence(s1);
				EXPECT_CALL(mock_groups[1], add(agent))
					.InSequence(s2);
				EXPECT_CALL(*agent, initLocation)
					.InSequence(s1, s2)
					.WillOnce(Invoke(&count_agents, &CountAgents::increase));

				// This is all the magic: using a common mapping, each process
				// only instantiate the agents located in local cells
				EXPECT_CALL(agent_factory, build)
					.WillOnce(Return(agent))
					.RetiresOnSaturation();
			}
		}

		ON_CALL(model, cells)
			.WillByDefault(Return(local_cells));
	}

	void TearDown() override {
		for(auto cell : local_cells)
			delete cell;
		for(auto agent : agents)
			delete agent;
	}
};

TEST_F(SpatialAgentBuilderTest, build) {
	using fpmas::api::scheduler::Job;

	// Expect distributedMoveAlgorithm execution
	Expectation move_algo_execution = EXPECT_CALL(
			mock_runtime, execute(
				(Matcher<const fpmas::api::scheduler::JobList&>) ElementsAre(
					Property(&std::reference_wrapper<const Job>::get,Ref(mock_job))
					)
				)
			)
		.After(add_to_temp_group);

	EXPECT_CALL(model, removeGroup(Ref(mock_move_group)))
		.After(move_algo_execution);

	builder.build(
			model,
			{mock_groups[0], mock_groups[1]},
			agent_factory, agent_mapping);
}

class EndConditionTest : public Test {
	protected:
		class Range : public NiceMock<MockRange<MockCell<>>> {
			public:
				int size;
				Range(int size)
					: size(size) {
						ON_CALL(*this, radius)
							.WillByDefault(ReturnPointee(&this->size));
					}
		};
		std::vector<fpmas::api::model::SpatialAgent<MockCell<>>*> agents;
		std::vector<Range*> mobility_ranges;
		std::vector<Range*> perception_ranges;
		fpmas::communication::MpiCommunicator comm;
		fpmas::random::DistributedGenerator<> rd;
		fpmas::random::UniformIntDistribution<> agent_count {5, 10};

		void SetUp() {
			int local_agent_count = agent_count(rd);
			for(int i = 0; i < local_agent_count; i++) {
				auto agent = new NiceMock<MockSpatialAgent<MockCell<>>>;
				auto mobility_range = new Range(4);
				auto perception_range = new Range(4);

				ON_CALL(*agent, mobilityRange)
					.WillByDefault(ReturnRef(*mobility_range));
				ON_CALL(*agent, perceptionRange)
					.WillByDefault(ReturnRef(*perception_range));

				agents.push_back(agent);
				mobility_ranges.push_back(mobility_range);
				perception_ranges.push_back(perception_range);
			}
		}
		void TearDown() {
			for(auto agent : agents)
				delete agent;
			for(auto range : mobility_ranges)
				delete range;
			for(auto range : perception_ranges)
				delete range;

		}

		void expectIterationCount(fpmas::api::model::EndCondition<MockCell<>>& end_condition, std::size_t expected_count) {
			end_condition.init(comm, agents, {});
			std::size_t counter = 0;
			while(!end_condition.end()) {
				end_condition.step();
				counter++;
			}
			ASSERT_EQ(counter, expected_count);
		}
};

TEST_F(EndConditionTest, dynamic_range) {
	fpmas::model::DynamicEndCondition<MockCell<>> end_condition;

	expectIterationCount(end_condition, 4);

	// Reduces all ranges
	for(auto range : mobility_ranges)
		range->size = 2;
	for(auto range : perception_ranges)
		range->size = 2;

	expectIterationCount(end_condition, 2);

	// Randomly updates all ranges
	fpmas::random::UniformIntDistribution<> range_size {0, 10};
	for(auto range : mobility_ranges)
		range->size = range_size(rd);
	for(auto range : perception_ranges)
		range->size = range_size(rd);

	// Sets ONE range to a bigger value
	FPMAS_ON_PROC(comm, 0)
		(*mobility_ranges.begin())->size = 12;

	// Iteration count must fit to the maximum range radius
	expectIterationCount(end_condition, 12);
}
