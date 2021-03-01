#include "fpmas/model/model.h"
#include "fpmas/model/serializer.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/zoltan_load_balancing.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/runtime/runtime.h"
#include "../mocks/model/mock_model.h"
#include "test_agents.h"
#include "utils/test.h"

#include <random>
#include <numeric>

using namespace testing;

class ModelGhostModeIntegrationTest : public Test {
	protected:
		FPMAS_DEFINE_GROUPS(G_1, G_2)

		static const int NODE_BY_PROC;
		static const int NUM_STEPS;
		fpmas::communication::MpiCommunicator comm;
		fpmas::model::detail::AgentGraph<fpmas::synchro::GhostMode> agent_graph {comm};
		fpmas::model::detail::ZoltanLoadBalancing lb {comm};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::detail::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::detail::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;
};
const int ModelGhostModeIntegrationTest::NODE_BY_PROC = 50;
const int ModelGhostModeIntegrationTest::NUM_STEPS = 100;

class IncreaseCount {
	private:
		std::unordered_map<DistributedId, unsigned long>& act_counts;
		DistributedId id;
	public:
		IncreaseCount(std::unordered_map<DistributedId, unsigned long>& act_counts, DistributedId id)
			: act_counts(act_counts), id(id) {}
		void operator()() {
			act_counts[id]++;
		}
};

class IncreaseCountAct : public fpmas::model::AgentNodeCallback {
	private:
		std::unordered_map<DistributedId, unsigned long>& act_counts;
	public:
		IncreaseCountAct(std::unordered_map<DistributedId, unsigned long>& act_counts) : act_counts(act_counts) {}

		void call(fpmas::model::AgentNode* node) override {
			if(node->data().get()->typeId() == DefaultMockAgentBase<1>::TYPE_ID) {
				EXPECT_CALL(*dynamic_cast<DefaultMockAgentBase<1>*>(node->data().get()), act)
					.Times(AnyNumber())
					.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
			} else if(node->data().get()->typeId() == DefaultMockAgentBase<10>::TYPE_ID) {
				EXPECT_CALL(*dynamic_cast<DefaultMockAgentBase<10>*>(node->data().get()), act)
					.Times(AnyNumber())
					.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
			}
		}
};

class ModelGhostModeIntegrationExecutionTest : public ModelGhostModeIntegrationTest {
	protected:

		fpmas::api::model::AgentGroup& group1 = model.buildGroup(G_1);
		fpmas::api::model::AgentGroup& group2 = model.buildGroup(G_2);
		void SetUp() override {

			agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new DefaultMockAgentBase<1>);
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.jobs());
			scheduler.schedule(0, 2, group2.jobs());
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, (unsigned int) id}] = 0;
				if(id % 2 == 0) {
					agent_types.insert({{0, (unsigned int) id}, typeid(DefaultMockAgentBase<1>)});
				} else {
					agent_types.insert({{0, (unsigned int) id}, typeid(DefaultMockAgentBase<10>)});
				}
			}
		}

		void checkExecutionCounts() {
			fpmas::communication::TypedMpi<int> mpi {agent_graph.getMpiCommunicator()};
			// TODO : MPI gather to compute the sum of act counts.
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				DistributedId dist_id {0, (unsigned int) id};
				std::vector<int> counts = mpi.gather(act_counts[dist_id], 0);
				if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
};

TEST_F(ModelGhostModeIntegrationExecutionTest, ghost_mode) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
	std::vector<fpmas::api::model::Agent*> group_1_agents;
	std::vector<fpmas::api::model::Agent*> group_2_agents;

	for(auto node : agent_graph.getNodes()) {
		fpmas::api::model::AgentPtr& agent = node.second->data();
		auto gids = agent->groupIds();
		if(std::count(gids.begin(), gids.end(), G_1)>0)
			group_1_agents.push_back(agent);
		else
			group_2_agents.push_back(agent);
	}
	ASSERT_THAT(group1.agents(), UnorderedElementsAreArray(group_1_agents));
	ASSERT_THAT(group2.agents(), UnorderedElementsAreArray(group_2_agents));
}

TEST_F(ModelGhostModeIntegrationExecutionTest, ghost_mode_with_link) {
	std::mt19937 engine;
	std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
	std::uniform_int_distribution<unsigned int> random_layer {0, 10};

	for(auto node : agent_graph.getNodes()) {
		for(int i = 0; i < NODE_BY_PROC / 2; i++) {
			DistributedId id {0, random_node(engine)};
			if(node.first.id() != id.id())
				agent_graph.link(node.second, agent_graph.getNode(id), 10);
		}
	}
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

class UpdateWeightAct : public fpmas::model::AgentNodeCallback {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight {0, 1};
	public:
		void call(fpmas::model::AgentNode* node) override {
			node->setWeight(random_weight(engine) * 10);
			EXPECT_CALL(*dynamic_cast<DefaultMockAgentBase<10>*>(node->data().get()), act)
				.Times(AnyNumber());
		}
};

class ModelGhostModeIntegrationLoadBalancingTest : public ModelGhostModeIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup(G_1);
			auto& group2 = model.buildGroup(G_2);

			agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			std::mt19937 engine;
			std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
			std::uniform_int_distribution<unsigned int> random_layer {0, 10};

			for(auto node : agent_graph.getNodes()) {
				for(int i = 0; i < NODE_BY_PROC / 2; i++) {
					DistributedId id {0, random_node(engine)};
					if(node.first.id() != id.id())
						agent_graph.link(node.second, agent_graph.getNode(id), 10);
				}
			}
			scheduler.schedule(0, 1, group1.jobs());
			scheduler.schedule(0, 2, group2.jobs());
			scheduler.schedule(0, 4, model.loadBalancingJob());
		}
};

TEST_F(ModelGhostModeIntegrationLoadBalancingTest, ghost_mode_dynamic_lb) {
	runtime.run(NUM_STEPS);
}

class ModelHardSyncModeIntegrationTest : public ::testing::Test {
	protected:
		FPMAS_DEFINE_GROUPS(G_1, G_2)

		static const int NODE_BY_PROC;
		static const int NUM_STEPS;
		fpmas::communication::MpiCommunicator comm;
		fpmas::model::detail::AgentGraph<fpmas::synchro::HardSyncMode> agent_graph {comm};
		fpmas::model::detail::ZoltanLoadBalancing lb {comm};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::detail::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::detail::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;
};
const int ModelHardSyncModeIntegrationTest::NODE_BY_PROC = 20;
const int ModelHardSyncModeIntegrationTest::NUM_STEPS = 100;

class ModelHardSyncModeIntegrationExecutionTest : public ModelHardSyncModeIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup(G_1);
			auto& group2 = model.buildGroup(G_2);

			agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new DefaultMockAgentBase<1>);
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.jobs());
			scheduler.schedule(0, 2, group2.jobs());
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, (unsigned int) id}] = 0;
				if(id % 2 == 0) {
					agent_types.insert({{0, (unsigned int) id}, typeid(DefaultMockAgentBase<1>)});
				} else {
					agent_types.insert({{0, (unsigned int) id}, typeid(DefaultMockAgentBase<10>)});
				}
			}
		}

		void checkExecutionCounts() {
			fpmas::communication::TypedMpi<int> mpi {agent_graph.getMpiCommunicator()};
			// TODO : MPI gather to compute the sum of act counts.
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				DistributedId dist_id {0, (unsigned int) id};
				std::vector<int> counts = mpi.gather(act_counts[dist_id], 0);
				if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
};

TEST_F(ModelHardSyncModeIntegrationExecutionTest, hard_sync_mode) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

TEST_F(ModelHardSyncModeIntegrationExecutionTest, hard_sync_mode_with_link) {
	std::mt19937 engine;
	std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
	std::uniform_int_distribution<unsigned int> random_layer {0, 10};

	for(auto node : agent_graph.getNodes()) {
		for(int i = 0; i < NODE_BY_PROC / 2; i++) {
			DistributedId id {0, random_node(engine)};
			if(node.first.id() != id.id())
				agent_graph.link(node.second, agent_graph.getNode(id), 10);
		}
	}
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

class ModelHardSyncModeIntegrationLoadBalancingTest : public ModelHardSyncModeIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup(G_1);
			auto& group2 = model.buildGroup(G_2);

			agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new DefaultMockAgentBase<10>);
				}
			}
			std::mt19937 engine;
			std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
			std::uniform_int_distribution<unsigned int> random_layer {0, 10};

			for(auto node : agent_graph.getNodes()) {
				for(int i = 0; i < NODE_BY_PROC / 2; i++) {
					DistributedId id {0, random_node(engine)};
					if(node.first.id() != id.id())
						agent_graph.link(node.second, agent_graph.getNode(id), 10);
				}
			}
			scheduler.schedule(0, 1, group1.jobs());
			scheduler.schedule(0, 2, group2.jobs());
			scheduler.schedule(0, 4, model.loadBalancingJob());
		}
};

TEST_F(ModelHardSyncModeIntegrationLoadBalancingTest, hard_sync_mode_dynamic_lb) {
	runtime.run(NUM_STEPS);
}

class HardSyncAgentModelIntegrationTest : public ::testing::Test {
	protected:
		FPMAS_DEFINE_GROUPS(G_0)

		static const unsigned int AGENT_BY_PROC = 50;
		static const unsigned int STEPS = 100;
		fpmas::communication::MpiCommunicator comm;
		fpmas::model::detail::AgentGraph<fpmas::synchro::HardSyncMode> agent_graph {comm};
		fpmas::model::detail::ZoltanLoadBalancing lb {comm};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::detail::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::detail::Model model {agent_graph, scheduler, runtime, scheduled_lb};
};

class HardSyncReadersModelIntegrationTest : public HardSyncAgentModelIntegrationTest {
	protected:
		fpmas::model::Behavior<ReaderAgent> read {&ReaderAgent::read};

		void SetUp() override {
			auto& group = model.buildGroup(G_0, read);
			FPMAS_ON_PROC(comm, 0) {
				for(unsigned int i = 0; i < AGENT_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new ReaderAgent);
				}

				std::mt19937 engine;
				std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
				for(auto node : agent_graph.getNodes()) {
					DistributedId id {0, random_node(engine)};
					if(node.first != id) {
						agent_graph.link(node.second, agent_graph.getNode(id), 0);
					}
				}
			}
			scheduler.schedule(0, 1, group.jobs());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(HardSyncReadersModelIntegrationTest, test) {
	runtime.run(STEPS);
}

class HardSyncWritersModelIntegrationTest : public HardSyncAgentModelIntegrationTest {
	protected:
		fpmas::model::Behavior<WriterAgent> write {&WriterAgent::write};

		void SetUp() override {
			auto& group = model.buildGroup(G_0, write);
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(unsigned int i = 0; i < AGENT_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new WriterAgent);
				}

				std::mt19937 engine;
				std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
				for(auto node : agent_graph.getNodes()) {
					DistributedId id {0, random_node(engine)};
					if(node.first != id) {
						agent_graph.link(node.second, agent_graph.getNode(id), 0);
					}
				}
			}
			scheduler.schedule(0, 1, group.jobs());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(HardSyncWritersModelIntegrationTest, test) {
	runtime.run(STEPS);

	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		const WriterAgent* agent = dynamic_cast<const WriterAgent*>(node.second->mutex()->read().get());
		ASSERT_EQ(agent->getCounter(), STEPS * node.second->getIncomingEdges().size());
	}
}

class ModelDynamicLinkGhostModeIntegrationTest : public ModelGhostModeIntegrationTest {
	protected:
		std::set<DistributedId> initial_links;
		fpmas::model::Behavior<LinkerAgent> link {&LinkerAgent::link};

		void SetUp() override {
			auto& group = model.buildGroup(G_1, link);

			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new LinkerAgent);
				}
				std::mt19937 engine;
				std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
				std::uniform_int_distribution<unsigned int> random_layer {0, 10};

				for(auto node : agent_graph.getNodes()) {
					for(int i = 0; i < NODE_BY_PROC / 2; i++) {
						DistributedId id {0, random_node(engine)};
						if(node.first.id() != id.id()) {
							auto edge = agent_graph.link(node.second, agent_graph.getNode(id), 10);
							initial_links.insert(edge->getId());
						}
					}
				}
			}
			scheduler.schedule(0, 1, group.jobs());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(ModelDynamicLinkGhostModeIntegrationTest, test) {
	runtime.run(NUM_STEPS);

	fpmas::communication::TypedMpi<unsigned int> mpi {agent_graph.getMpiCommunicator()};

	// Build local links set
	std::set<DistributedId> links;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
		links.insert(agent->links.begin(), agent->links.end());
	}

	// Build local unlinks set
	std::set<DistributedId> unlinks;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
		unlinks.insert(agent->unlinks.begin(), agent->unlinks.end());
	}

	// Gather all edges id in the complete distributed graph
	fpmas::communication::TypedMpi<std::set<DistributedId>> edge_ids_mpi {agent_graph.getMpiCommunicator()};
	std::set<DistributedId> edge_ids;
	for(auto edge : agent_graph.getEdges()) {
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
		
	ASSERT_THAT(final_edge_id_set, ::testing::UnorderedElementsAreArray(total_links));

	FPMAS_ON_PROC(comm, 0) {
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Initial link count : %lu",
				initial_links.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Link count : %lu - Unlink count : %lu",
				total_links.size(), total_unlinks.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Total edge count : %lu", edge_count);
	}
}

class ModelDynamicLinkHardSyncModeIntegrationTest : public ModelHardSyncModeIntegrationTest {
	protected:
		std::set<DistributedId> initial_links;

		void SetUp() override {
			auto& group = model.buildGroup(G_1);

			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group.add(new LinkerAgent);
				}
				std::mt19937 engine;
				std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
				std::uniform_int_distribution<unsigned int> random_layer {0, 10};

				for(auto node : agent_graph.getNodes()) {
					for(int i = 0; i < NODE_BY_PROC / 2; i++) {
						DistributedId id {0, random_node(engine)};
						if(node.first.id() != id.id()) {
							auto edge = agent_graph.link(node.second, agent_graph.getNode(id), 10);
							initial_links.insert(edge->getId());
						}
					}
				}
			}
			scheduler.schedule(0, 1, group.jobs());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(ModelDynamicLinkHardSyncModeIntegrationTest, test) {
	runtime.run(NUM_STEPS);

	fpmas::communication::TypedMpi<unsigned int> mpi {agent_graph.getMpiCommunicator()};

	// Build local links set
	std::set<DistributedId> links;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
		links.insert(agent->links.begin(), agent->links.end());
	}

	// Build local unlinks set
	std::set<DistributedId> unlinks;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = dynamic_cast<const LinkerAgent*>(node.second->mutex()->read().get());
		unlinks.insert(agent->unlinks.begin(), agent->unlinks.end());
	}

	// Gather all edges id in the global distributed graph
	fpmas::communication::TypedMpi<std::set<DistributedId>> edge_ids_mpi {agent_graph.getMpiCommunicator()};
	std::set<DistributedId> edge_ids;
	for(auto edge : agent_graph.getEdges()) {
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

	FPMAS_ON_PROC(comm, 0) {
		// Builds links list
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

		ASSERT_THAT(final_edge_id_set, ::testing::UnorderedElementsAreArray(total_links));

		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Initial link count : %lu",
				initial_links.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Link count : %lu - Unlink count : %lu",
				total_links.size(), total_unlinks.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Total edge count : %lu", edge_count);
	}
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
	//FPMAS_MIN_PROCS("ModelDynamicGroupIntegration.migration_edge_case", model.getMpiCommunicator(), 2) {
		ASSERT_THAT(init_group.localAgents(), SizeIs(1));

		other_group.add(*init_group.localAgents().begin());
		//init_group.clear();
		init_group.remove(*init_group.localAgents().begin());

		fpmas::graph::PartitionMap partition;
		partition[(*other_group.localAgents().begin())->node()->getId()]
			= (model.getMpiCommunicator().getRank()+1) % model.getMpiCommunicator().getSize();

		model.graph().distribute(partition);

		ASSERT_THAT(other_group.localAgents(), SizeIs(1));
		ASSERT_THAT((*other_group.localAgents().begin())->groups(), ElementsAre(&other_group));
	//}
}
