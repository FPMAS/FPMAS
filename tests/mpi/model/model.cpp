#include "fpmas/model/model.h"
#include "fpmas/model/serializer.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/zoltan_load_balancing.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/runtime/runtime.h"
#include "model/mock_model.h"
#include "test_agents.h"
#include "utils/test.h"

#include <random>
#include <numeric>

/*
 * - model_execution_test
 * - model_load_balancing_test
 * - hard_sync_mode_read_write
 * - model_dynamic_link_test
 */

using namespace testing;

FPMAS_DEFINE_GROUPS(G_0, G_1, G_2);

static const FPMAS_ID_TYPE NODE_BY_PROC = 50;
static const int NUM_STEPS = 100;

template<template<typename> class SynchronizationMode>
class ModelIntegrationTest : public Test {
	protected:
		fpmas::communication::MpiCommunicator comm;
		fpmas::model::detail::AgentGraph<SynchronizationMode> agent_graph {comm};
		fpmas::model::detail::ZoltanLoadBalancing lb {comm};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::detail::ScheduledLoadBalancing scheduled_lb {
			lb, scheduler, runtime
		};

		fpmas::model::detail::Model model {agent_graph, scheduler, runtime, scheduled_lb};
};


class IncreaseCountAct : public fpmas::api::model::SetAgentLocalCallback {
	private:
		// Stores the count of time each agent as been executed
		std::unordered_map<DistributedId, unsigned long>& act_counts;
	public:
		IncreaseCountAct(std::unordered_map<DistributedId, unsigned long>& act_counts)
			: act_counts(act_counts) {}

		void call(const fpmas::api::model::SetAgentLocalEvent& event) override {
			DistributedId agent_id = event.node->getId();
			if(event.node->data().get()->typeId() == DefaultMockAgentBase<1>::TYPE_ID) {
				// When DefaultMockAgentBase::act() is called, update
				// act_counts
				EXPECT_CALL(*static_cast<DefaultMockAgentBase<1>*>(event.node->data().get()), act)
					.Times(AnyNumber())
					.WillRepeatedly(Invoke([this, agent_id] () {
								this->act_counts[agent_id]++;
								}));
			} else if(event.node->data().get()->typeId() == DefaultMockAgentBase<10>::TYPE_ID) {
				// Same
				EXPECT_CALL(*static_cast<DefaultMockAgentBase<10>*>(event.node->data().get()), act)
					.Times(AnyNumber())
					.WillRepeatedly(Invoke([this, agent_id] () {
								this->act_counts[agent_id]++;
								}));
			}
		}
};

/************************/
/* model_execution_test */
/************************/
template<template<typename> class SynchronizationMode>
class ModelExecutionTest :
	public ModelIntegrationTest<SynchronizationMode> {
	protected:

		fpmas::api::model::AgentGroup& group1 = this->model.buildGroup(G_1);
		fpmas::api::model::AgentGroup& group2 = this->model.buildGroup(G_2);

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;

		void SetUp() override {

			// Activates MockAgent act() behavior when set local
			this->agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			FPMAS_ON_PROC(this->comm, 0) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new DefaultMockAgentBase<1>);
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			this->scheduler.schedule(0, 1, group1.jobs());
			this->scheduler.schedule(0, 2, group2.jobs());
			for(FPMAS_ID_TYPE id = 0; id < 2 * NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, id}] = 0;
				if(id % 2 == 0) {
					agent_types.insert({{0, id}, typeid(DefaultMockAgentBase<1>)});
				} else {
					agent_types.insert({{0, id}, typeid(DefaultMockAgentBase<10>)});
				}
			}
		}

		void checkExecutionCounts() {
			fpmas::communication::TypedMpi<int> mpi {this->comm};
			// TODO : MPI gather to compute the sum of act counts.
			for(FPMAS_ID_TYPE id = 0; id < 2 * NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); id++) {
				DistributedId dist_id {0, id};
				std::vector<int> counts = mpi.gather(act_counts[dist_id], 0);
				if(this->agent_graph.getMpiCommunicator().getRank() == 0) {
					int sum = 0;
					sum = std::accumulate(counts.begin(), counts.end(), sum);
					if(agent_types.at(dist_id) == typeid(DefaultMockAgentBase<1>)) {
						// Executed every step
						ASSERT_EQ(sum, NUM_STEPS);
					} else {
						// Executed one in two step
						ASSERT_EQ(sum, NUM_STEPS / 2);
					}
				}
			}
		}

		void buildLinks() {
			std::mt19937 engine;
			std::uniform_int_distribution<FPMAS_ID_TYPE> random_node {
				0, static_cast<FPMAS_ID_TYPE>(this->agent_graph.getNodes().size()-1)};
			std::uniform_int_distribution<unsigned int> random_layer {0, 10};

			for(auto node : this->agent_graph.getNodes()) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC / 2; i++) {
					DistributedId id {0, random_node(engine)};
					if(node.first.id() != id.id())
						this->agent_graph.link(node.second, this->agent_graph.getNode(id), 10);
				}
			}
		}
};

typedef ModelExecutionTest<fpmas::synchro::GhostMode> ModelGhostModeExecutionTest;

TEST_F(ModelGhostModeExecutionTest, test) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

TEST_F(ModelGhostModeExecutionTest, test_with_link) {
	buildLinks();

	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

typedef ModelExecutionTest<fpmas::synchro::hard::hard_link::HardSyncMode> ModelHardSyncModeExecutionTest;

TEST_F(ModelHardSyncModeExecutionTest, test) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

TEST_F(ModelHardSyncModeExecutionTest, test_with_link) {
	buildLinks();

	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

typedef ModelExecutionTest<fpmas::synchro::hard::ghost_link::HardSyncMode> ModelHardSyncModeWithGhostLinkExecutionTest;

TEST_F(ModelHardSyncModeWithGhostLinkExecutionTest, test) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

TEST_F(ModelHardSyncModeWithGhostLinkExecutionTest, test_with_link) {
	buildLinks();

	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

/*****************************/
/* model_load_balancing_test */
/*****************************/
class UpdateWeightAct : public fpmas::api::model::SetAgentLocalCallback {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight {0, 1};
	public:
		void call(const fpmas::api::model::SetAgentLocalEvent& event) override {
			event.node->setWeight(random_weight(engine) * 10);
			EXPECT_CALL(*dynamic_cast<DefaultMockAgentBase<10>*>(event.node->data().get()), act)
				.Times(AnyNumber());
		}
};

template<template<typename> class SynchonizationMode>
class ModelLoadBalancingTest : public ModelIntegrationTest<SynchonizationMode> {
	protected:
		void SetUp() override {
			auto& group1 = this->model.buildGroup(G_1);
			auto& group2 = this->model.buildGroup(G_2);

			this->agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			FPMAS_ON_PROC(this->comm, 0) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			std::mt19937 engine;
			std::uniform_int_distribution<FPMAS_ID_TYPE> random_node {
				0, static_cast<FPMAS_ID_TYPE>(this->agent_graph.getNodes().size()-1)};
			std::uniform_int_distribution<unsigned int> random_layer {0, 10};

			for(auto node : this->agent_graph.getNodes()) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC / 2; i++) {
					DistributedId id {0, random_node(engine)};
					if(node.first.id() != id.id())
						this->agent_graph.link(node.second, this->agent_graph.getNode(id), 10);
				}
			}
			this->scheduler.schedule(0, 1, group1.jobs());
			this->scheduler.schedule(0, 2, group2.jobs());
			this->scheduler.schedule(0, 4, this->model.loadBalancingJob());
		}
};

typedef ModelLoadBalancingTest<fpmas::synchro::GhostMode> ModelGhostModeLoadBalancingTest;
TEST_F(ModelGhostModeLoadBalancingTest, dynamic_lb) {
	runtime.run(NUM_STEPS);
}

typedef ModelLoadBalancingTest<fpmas::synchro::hard::hard_link::HardSyncMode> ModelHardSyncModeLoadBalancingTest;
TEST_F(ModelHardSyncModeLoadBalancingTest, dynamic_lb) {
	runtime.run(NUM_STEPS);
}

typedef ModelLoadBalancingTest<fpmas::synchro::hard::ghost_link::HardSyncMode> ModelHardSyncModeGhostLinkLoadBalancingTest;
TEST_F(ModelHardSyncModeGhostLinkLoadBalancingTest, dynamic_lb) {
	runtime.run(NUM_STEPS);
}

/*****************************/
/* hard_sync_mode_read_write */
/*****************************/
template<template<typename> class SynchonizationMode>
class ReadersModelTest :
	public ModelIntegrationTest<SynchonizationMode> {
	protected:
		fpmas::model::Behavior<ReaderAgent> read {&ReaderAgent::read};

		void SetUp() override {
			auto& group = this->model.buildGroup(G_0, read);
			FPMAS_ON_PROC(this->comm, 0) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new ReaderAgent);
				}

				std::mt19937 engine;
				std::uniform_int_distribution<FPMAS_ID_TYPE> random_node {
					0, static_cast<FPMAS_ID_TYPE>(this->agent_graph.getNodes().size()-1)};
				for(auto node : this->agent_graph.getNodes()) {
					DistributedId id {0, random_node(engine)};
					if(node.first != id) {
						this->agent_graph.link(node.second, this->agent_graph.getNode(id), 0);
					}
				}
			}

			this->scheduler.schedule(0, 1, group.jobs());
			this->scheduler.schedule(0, 10, this->model.loadBalancingJob());
		}
};

typedef ReadersModelTest<fpmas::synchro::hard::hard_link::HardSyncMode> HardSyncModeReadersModelTest;
TEST_F(HardSyncModeReadersModelTest, test) {
	runtime.run(NUM_STEPS);
}

typedef ReadersModelTest<fpmas::synchro::hard::ghost_link::HardSyncMode> HardSyncModeGhostLinkReadersModelTest;
TEST_F(HardSyncModeGhostLinkReadersModelTest, test) {
	runtime.run(NUM_STEPS);
}

template<template<typename> class SynchronizationMode>
class WritersModelTest :
	public ModelIntegrationTest<SynchronizationMode> {
	protected:
		fpmas::model::Behavior<WriterAgent> write {&WriterAgent::write};

		void SetUp() override {
			auto& group = this->model.buildGroup(G_0, write);
			if(this->comm.getRank() == 0) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new WriterAgent);
				}

				std::mt19937 engine;
				std::uniform_int_distribution<FPMAS_ID_TYPE> random_node {
					0, static_cast<FPMAS_ID_TYPE>(this->agent_graph.getNodes().size()-1)};
				for(auto node : this->agent_graph.getNodes()) {
					DistributedId id {0, random_node(engine)};
					if(node.first != id) {
						this->agent_graph.link(node.second, this->agent_graph.getNode(id), 0);
					}
				}
			}
			this->scheduler.schedule(0, 1, group.jobs());
			this->scheduler.schedule(0, 10, this->model.loadBalancingJob());
		}

		void checkWrites() {
			for(auto node : this->agent_graph.getLocationManager().getLocalNodes()) {
				const WriterAgent* agent = dynamic_cast<const WriterAgent*>(node.second->mutex()->read().get());
				ASSERT_EQ(agent->getCounter(), NUM_STEPS * node.second->getIncomingEdges().size());
			}
		}
};

typedef WritersModelTest<fpmas::synchro::hard::hard_link::HardSyncMode> HardSyncModeWritersModelTest;
TEST_F(HardSyncModeWritersModelTest, test) {
	runtime.run(NUM_STEPS);
	checkWrites();
}

typedef WritersModelTest<fpmas::synchro::hard::ghost_link::HardSyncMode> HardSyncModeGhostLinkWritersModelTest;
TEST_F(HardSyncModeGhostLinkWritersModelTest, test) {
	runtime.run(NUM_STEPS);
}

/***************************/
/* model_dynamic_link_test */
/***************************/
template<template<typename> class SynchonizationMode>
class ModelDynamicLinkTest : public ModelIntegrationTest<SynchonizationMode> {
	protected:
		std::set<DistributedId> initial_links;
		fpmas::model::Behavior<LinkerAgent> link {&LinkerAgent::link};

		void SetUp() override {
			auto& group = this->model.buildGroup(G_1, link);

			FPMAS_ON_PROC(this->comm, 0) {
				for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC * this->agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new LinkerAgent);
				}
				std::mt19937 engine;
				std::uniform_int_distribution<FPMAS_ID_TYPE> random_node {
					0, static_cast<FPMAS_ID_TYPE>(this->agent_graph.getNodes().size()-1)};
				std::uniform_int_distribution<fpmas::graph::LayerId> random_layer {0, 10};

				for(auto node : this->agent_graph.getNodes()) {
					for(FPMAS_ID_TYPE i = 0; i < NODE_BY_PROC / 2; i++) {
						DistributedId id {0, random_node(engine)};
						if(node.first.id() != id.id()) {
							auto edge = this->agent_graph.link(
									node.second, this->agent_graph.getNode(id), 10);
							initial_links.insert(edge->getId());
						}
					}
				}
			}
			this->scheduler.schedule(0, 1, group.jobs());
			this->scheduler.schedule(0, 10, this->model.loadBalancingJob());
		}

		void checkLinks() {
			// Build local links set
			std::set<DistributedId> links;
			for(auto node : this->agent_graph.getLocationManager().getLocalNodes()) {
				auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
				links.insert(agent->links.begin(), agent->links.end());
			}

			// Build local unlinks set
			std::set<DistributedId> unlinks;
			for(auto node : this->agent_graph.getLocationManager().getLocalNodes()) {
				auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
				unlinks.insert(agent->unlinks.begin(), agent->unlinks.end());
			}

			// Gather all edges id in the complete distributed graph
			fpmas::communication::TypedMpi<std::set<DistributedId>> edge_ids_mpi {
				this->comm
			};
			std::set<DistributedId> edge_ids;
			for(auto edge : this->agent_graph.getEdges()) {
				edge_ids.insert(edge.first);
			}
			std::vector<std::set<DistributedId>> edge_id_sets = edge_ids_mpi.gather(edge_ids, 0);
			std::set<DistributedId> final_edge_id_set;
			for(auto set : edge_id_sets)
				for(auto id : set)
					final_edge_id_set.insert(id);

			// Migrates links / unlinks
			std::vector<std::set<DistributedId>> recv_total_links = edge_ids_mpi.gather(links, 0);
			std::vector<std::set<DistributedId>> recv_total_unlinks = edge_ids_mpi.gather(unlinks, 0);
			unsigned int edge_count = final_edge_id_set.size();

			// Buils links list
			std::set<DistributedId> total_links;
			for(auto set : recv_total_links)
				total_links.insert(set.begin(), set.end());

			// Builds unlinks list
			std::set<DistributedId> total_unlinks;
			for(auto set : recv_total_unlinks)
				total_unlinks.insert(set.begin(), set.end());

			// Builds the expected edge ids list
			for(auto id : initial_links)
				total_links.insert(id);
			for(auto id : total_unlinks)
				total_links.erase(id);

			ASSERT_THAT(final_edge_id_set, UnorderedElementsAreArray(total_links));

			FPMAS_ON_PROC(this->comm, 0) {
				FPMAS_LOGI(this->comm.getRank(),
						"DYNAMIC_LINK_TEST", "Initial link count : %lu",
						initial_links.size());
				FPMAS_LOGI(this->comm.getRank(),
						"DYNAMIC_LINK_TEST", "Link count : %lu - Unlink count : %lu",
						total_links.size(), total_unlinks.size());
				FPMAS_LOGI(this->comm.getRank(),
						"DYNAMIC_LINK_TEST", "Total edge count : %lu", edge_count);
			}
		}
};

typedef ModelDynamicLinkTest<fpmas::synchro::GhostMode> ModelDynamicLinkGhostModeTest;

TEST_F(ModelDynamicLinkGhostModeTest, test) {
	runtime.run(NUM_STEPS);
	checkLinks();
}

typedef ModelDynamicLinkTest<fpmas::synchro::hard::hard_link::HardSyncMode>
ModelDynamicLinkHardSyncModeTest;

TEST_F(ModelDynamicLinkHardSyncModeTest, test) {
	runtime.run(NUM_STEPS);
	checkLinks();
}

typedef ModelDynamicLinkTest<fpmas::synchro::hard::ghost_link::HardSyncMode> ModelDynamicLinkHardSyncModeGhostLinkTest;

TEST_F(ModelDynamicLinkHardSyncModeGhostLinkTest, test) {
	runtime.run(NUM_STEPS);
	checkLinks();
}

TEST(DefaultModelConfig, build) {
	fpmas::model::Model<fpmas::synchro::HardSyncMode> model;
}

class ModelDynamicGroupIntegration : public Test {
	protected:
		fpmas::model::Model<fpmas::synchro::GhostMode> model;

		fpmas::api::model::AgentGroup& init_group {model.buildGroup(0)};
		fpmas::api::model::AgentGroup& other_group {model.buildGroup(1)};

		void SetUp() override {
			fpmas::graph::PartitionMap partition;
			FPMAS_ON_PROC(model.getMpiCommunicator(), 0) {
				std::vector<fpmas::api::model::Agent*> agents;
				for(int i = 0; i < model.getMpiCommunicator().getSize(); i++) {
					auto agent = new BasicAgent;
					init_group.add(agent);
					partition[agent->node()->getId()] = i;
					agents.push_back(agent);
				}
				for(std::size_t i = 0; i < agents.size(); i++)
					model.link(agents[i], agents[(i+1) % agents.size()], 0);
			}
			model.graph().distribute(partition);
		}
};

TEST_F(ModelDynamicGroupIntegration, migration_edge_case) {
	// Adds the agent to other_group, so that it still belong to at least one
	// group
	other_group.add(*init_group.localAgents().begin());
	// Removes the agent from the group on process i
	init_group.remove(*init_group.localAgents().begin());

	// Migrates the agent to process i+1, where a DISTANT representation of the
	// agent was already contained in the graph, but the agent was only
	// contained in init_group
	fpmas::graph::PartitionMap partition;
	partition[(*other_group.localAgents().begin())->node()->getId()]
		= (model.getMpiCommunicator().getRank()+1) % model.getMpiCommunicator().getSize();

	model.graph().distribute(partition);

	ASSERT_THAT(init_group.localAgents(), SizeIs(0));
	ASSERT_THAT(other_group.localAgents(), SizeIs(1));
	ASSERT_THAT(
			(*other_group.localAgents().begin())->groups(),
			ElementsAre(&other_group)
			);
}

TEST(DistributedAgentNodeBuilder, test) {
	static const std::size_t N_AGENT = 1000;

	fpmas::model::Model<fpmas::synchro::GhostMode> model;
	fpmas::model::Behavior<ReaderAgent> reader_behavior(&ReaderAgent::read);

	auto& group = model.buildGroup(0, reader_behavior);

	fpmas::random::DistributedGenerator<> generator;
	fpmas::random::PoissonDistribution<std::size_t> edge_dist(6);
	fpmas::graph::DistributedUniformGraphBuilder<fpmas::model::AgentPtr> builder(
			generator, edge_dist
			);

	fpmas::model::DistributedAgentNodeBuilder node_builder(
			group, N_AGENT, []() {return new ReaderAgent;}, model.getMpiCommunicator()
			);
	builder.build(node_builder, 0, model.graph());

	model.graph().synchronize();
	model.runtime().execute(model.loadBalancingJob());

	fpmas::communication::TypedMpi<std::size_t> mpi(model.getMpiCommunicator());
	std::size_t total_agent_count = fpmas::communication::all_reduce(
			mpi, group.localAgents().size()
			);
	ASSERT_EQ(total_agent_count, N_AGENT);

	model.scheduler().schedule(0, 1, group.jobs());
	model.runtime().run(100);
}
