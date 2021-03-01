#include "fpmas/model/model.h"
#include "fpmas/io/breakpoint.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "../model/test_agents.h"

#include "gmock/gmock.h"

using namespace testing;

FPMAS_DEFINE_GROUPS(HELLO, READ, WRITE)

class ModelBreakpointTest : public Test {
	protected:
		static const std::size_t num_readers = 50;
		static const std::size_t num_writers = 50;
		static const std::size_t num_edges = 300;

		/*
		 * A test model, based on Reader and Writer agents. See
		 * tests/mpi/model/test_agents.h for more information.
		 */
		struct TestModel : public fpmas::model::Model<fpmas::synchro::HardSyncMode> {
			fpmas::model::Behavior<ReaderWriterBase> hello {&ReaderWriterBase::hello};
			fpmas::model::Behavior<ReaderAgent> read {&ReaderAgent::read};
			fpmas::model::Behavior<WriterAgent> write {&WriterAgent::write};

			fpmas::api::model::AgentGroup& hello_group = this->buildGroup(HELLO, hello);
			fpmas::api::model::AgentGroup& readers = this->buildGroup(READ, read);
			fpmas::api::model::AgentGroup& writers = this->buildGroup(WRITE, write);

			TestModel() {
				// Initializes the model only on proc 0
				FPMAS_ON_PROC(this->getMpiCommunicator(), 0) {
					for(std::size_t i = 0; i < num_readers; i++) {
						auto agent = new ReaderAgent;
						readers.add(agent);
						hello_group.add(agent);
					}
					for(std::size_t i = 0; i < num_writers; i++) {
						auto agent = new WriterAgent;
						writers.add(agent);
						hello_group.add(agent);
					}

					fpmas::random::mt19937 rd;
					fpmas::random::UniformIntDistribution<std::size_t> rd_agent(0, hello_group.localAgents().size()-1);
					for(std::size_t i = 0; i < num_edges; i++) {
						auto src = hello_group.localAgents().at(rd_agent(rd));
						auto tgt = hello_group.localAgents().at(rd_agent(rd));
						if(src != tgt)
							this->link(src, tgt, 0);
					}
				}
				this->scheduler().schedule(0, 1, hello_group.jobs());
				this->scheduler().schedule(0.1, 1, readers.jobs());
				this->scheduler().schedule(0.2, 1, writers.jobs());

				// Distributes the model
				this->runtime().execute(this->loadBalancingJob());
			}

			/*
			 * Utility method used to build a representation of the current
			 * model state
			 */
			void snapshot(
					std::unordered_map<DistributedId, int> agent_data,
					std::set<DistributedId> hello_agents,
					std::set<DistributedId> reader_agents,
					std::set<DistributedId> writer_agents) {
				for(auto agent : hello_group.agents()) {
					agent_data[agent->node()->getId()] = dynamic_cast<ReaderWriterBase*>(agent)->getCounter();
					hello_agents.insert(agent->node()->getId());
				}
				for(auto agent : readers.agents())
					reader_agents.insert(agent->node()->getId());
				for(auto agent : writers.agents())
					writer_agents.insert(agent->node()->getId());
			}
		};

		/*
		 * Reference model. This model is run normally from timestep 0 to 100.
		 */
		TestModel reference;
		/*
		 * Breakpoint model. Same configuration as `reference`, but is executed
		 * from 0 to 50, dumped, cleared, loaded, and executed from 50 to 100.
		 */
		TestModel breakpoint_model;
};

TEST_F(ModelBreakpointTest, test) {

	// Runs the reference model
	reference.runtime().run(0, 100);
	// Gathers final agent data for the reference model
	std::unordered_map<DistributedId, int> reference_data;
	for(auto agent : reference.hello_group.localAgents())
		reference_data[agent->node()->getId()] = dynamic_cast<ReaderWriterBase*>(agent)->getCounter();

	fpmas::communication::TypedMpi<decltype(reference_data)> mpi (reference.getMpiCommunicator());


	// Gathers agent data on proc 0 (so that the test is distribution
	// independent)
	auto data = mpi.gather(reference_data, 0); // Vector of maps
	// Merge data
	for(auto map : data)
		for(auto item : map) 
			reference_data.insert(item);

	// Runs the breakpoint model from timestep 0 to 50
	breakpoint_model.runtime().run(0, 50);

	// Performs a breakpoint on `breakpoint_model`
	fpmas::io::Breakpoint<fpmas::api::model::Model> breakpoint;
	{
		// Build a model snapshot before dump
		std::unordered_map<DistributedId, int> dump_agent_data;
		std::set<DistributedId> dump_hello_agents;
		std::set<DistributedId> dump_reader_agents;
		std::set<DistributedId> dump_writer_agents;

		breakpoint_model.snapshot(dump_agent_data, dump_hello_agents, dump_reader_agents, dump_writer_agents);

		// Dumps the model
		std::stringstream stream;
		breakpoint.dump(stream, breakpoint_model);

		// Reinitializes the model
		breakpoint_model.clear();
		ASSERT_THAT(breakpoint_model.hello_group.agents(), IsEmpty());
		ASSERT_THAT(breakpoint_model.readers.agents(), IsEmpty());
		ASSERT_THAT(breakpoint_model.writers.agents(), IsEmpty());

		// Loads the model from the previous dump
		breakpoint.load(stream, breakpoint_model);

		// Builds a snapshot of the loaded model
		std::unordered_map<DistributedId, int> load_agent_data;
		std::set<DistributedId> load_hello_agents;
		std::set<DistributedId> load_reader_agents;
		std::set<DistributedId> load_writer_agents;
		breakpoint_model.snapshot(load_agent_data, load_hello_agents, load_reader_agents, load_writer_agents);

		// The state of the loaded model must be the same as the dumped model
		ASSERT_THAT(load_agent_data, UnorderedElementsAreArray(dump_agent_data));
		ASSERT_THAT(load_hello_agents, UnorderedElementsAreArray(dump_hello_agents));
		ASSERT_THAT(load_reader_agents, UnorderedElementsAreArray(dump_reader_agents));
		ASSERT_THAT(load_writer_agents, UnorderedElementsAreArray(dump_writer_agents));
	}

	// Resumes execution
	breakpoint_model.runtime().run(50, 100);

	// Gathers final agent data for the breakpoint model
	std::unordered_map<DistributedId, int> breakpoint_data;
	for(auto agent : breakpoint_model.hello_group.localAgents())
		breakpoint_data[agent->node()->getId()] = dynamic_cast<ReaderWriterBase*>(agent)->getCounter();

	// Gather breakpoint data
	data = mpi.gather(breakpoint_data, 0); // Vector of maps
	// Merge data
	for(auto map : data)
		for(auto item : map) 
			breakpoint_data.insert(item);

	// Assert that reference data == breakpoint data, only on proc 0 (where
	// data is gathered)
	// The result of the execution of the model, with or without breakpoint,
	// must be the same (considering the behaviors of Reader and Writer agents,
	// see tests/mpi/model/test_agents.h).
	FPMAS_ON_PROC(reference.getMpiCommunicator(), 0) {
		ASSERT_THAT(breakpoint_data, UnorderedElementsAreArray(reference_data));
	}
}
