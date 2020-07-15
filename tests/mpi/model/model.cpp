#include "fpmas/model/model.h"
#include "fpmas/model/serializer.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/load_balancing/zoltan_load_balancing.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/runtime/runtime.h"
#include "../mocks/model/mock_model.h"

#include <random>
#include <numeric>

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::InvokeWithoutArgs;
using ::testing::UnorderedElementsAreArray;

class ReaderAgent;
class WriterAgent;
class LinkerAgent;

FPMAS_JSON_SERIALIZE_AGENT(ReaderAgent, WriterAgent, LinkerAgent, MockAgentBase<1>, MockAgentBase<10>)
FPMAS_DEFAULT_JSON(MockAgentBase<1>)
FPMAS_DEFAULT_JSON(MockAgentBase<10>)

class ModelGhostModeIntegrationTest : public ::testing::Test {
	protected:
		static const int NODE_BY_PROC;
		static const int NUM_STEPS;
		fpmas::model::AgentGraph<fpmas::synchro::GhostMode> agent_graph;
		fpmas::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

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
			switch(node->data().get()->typeId()) {
				case MockAgentBase<1>::TYPE_ID :
					EXPECT_CALL(*static_cast<MockAgentBase<1>*>(node->data().get()), act)
						.Times(AnyNumber())
						.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
					break;
				case MockAgentBase<10>::TYPE_ID :
					EXPECT_CALL(*static_cast<MockAgentBase<10>*>(node->data().get()), act)
						.Times(AnyNumber())
						.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
					break;
				default:
					break;
			}
		}
};

class ModelGhostModeIntegrationExecutionTest : public ModelGhostModeIntegrationTest {
	protected:
		fpmas::api::model::AgentGroup& group1 = model.buildGroup();
		fpmas::api::model::AgentGroup& group2 = model.buildGroup();
		void SetUp() override {

			agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new MockAgentBase<1>);
					group2.add(new MockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, (unsigned int) id}] = 0;
				if(id % 2 == 0) {
					agent_types[{0, (unsigned int) id}] = 1;
				} else {
					agent_types[{0, (unsigned int) id}] = 10;
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
					if(agent_types[dist_id] == 1) {
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
	std::vector<fpmas::api::model::AgentPtr*> group_1_agents;
	std::vector<fpmas::api::model::AgentPtr*> group_2_agents;

	for(auto node : agent_graph.getNodes()) {
		fpmas::api::model::AgentPtr& agent = node.second->data();
		if(agent->group() == &group1)
			group_1_agents.push_back(&agent);
		else
			group_2_agents.push_back(&agent);
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
			EXPECT_CALL(*static_cast<MockAgentBase<10>*>(node->data().get()), act)
				.Times(AnyNumber());
		}
};

class ModelGhostModeIntegrationLoadBalancingTest : public ModelGhostModeIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new MockAgentBase<10>);
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
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			scheduler.schedule(0, 4, model.loadBalancingJob());
		}
};

TEST_F(ModelGhostModeIntegrationLoadBalancingTest, ghost_mode_dynamic_lb) {
	runtime.run(NUM_STEPS);
}

class ReaderAgent : public fpmas::model::AgentBase<ReaderAgent, 0> {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight;
		int counter = 0;
	public:
		void act() override {
			FPMAS_LOGD(node()->getLocation(), "READER_AGENT", "Execute agent %s - count : %i", ID_C_STR(node()->getId()), counter);
			for(auto neighbour : node()->outNeighbors()) {
				ASSERT_THAT(static_cast<const ReaderAgent*>(neighbour->mutex().read().get())->getCounter(), Ge(counter));
			}
			counter++;
			node()->setWeight(random_weight(engine) * 10);
		}

		void setCounter(int count) {counter = count;}

		int getCounter() const {
			return counter;
		}
};

void to_json(nlohmann::json& j, const fpmas::api::utils::VirtualPtrWrapper<ReaderAgent>& agent) {
	j["c"] = agent->getCounter();
}

void from_json(const nlohmann::json& j, fpmas::api::utils::VirtualPtrWrapper<ReaderAgent>& agent) {
	ReaderAgent* agent_ptr = new ReaderAgent;
	agent_ptr->setCounter(j.at("c").get<int>());
	agent = {agent_ptr};
}

class ModelHardSyncModeIntegrationTest : public ::testing::Test {
	protected:
		static const int NODE_BY_PROC;
		static const int NUM_STEPS;
		fpmas::model::AgentGraph<fpmas::synchro::HardSyncMode> agent_graph;
		fpmas::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;

};
const int ModelHardSyncModeIntegrationTest::NODE_BY_PROC = 20;
const int ModelHardSyncModeIntegrationTest::NUM_STEPS = 100;

class ModelHardSyncModeIntegrationExecutionTest : public ModelHardSyncModeIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new MockAgentBase<1>);
					group2.add(new MockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, (unsigned int) id}] = 0;
				if(id % 2 == 0) {
					agent_types[{0, (unsigned int) id}] = 1;
				} else {
					agent_types[{0, (unsigned int) id}] = 10;
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
					if(agent_types[dist_id] == 1) {
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
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new MockAgentBase<10>);
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
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			scheduler.schedule(0, 4, model.loadBalancingJob());
		}
};

TEST_F(ModelHardSyncModeIntegrationLoadBalancingTest, hard_sync_mode_dynamic_lb) {
	runtime.run(NUM_STEPS);
}

class WriterAgent : public fpmas::model::AgentBase<WriterAgent, 2> {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight;
		int counter = 0;
	public:
		void act() override {
			FPMAS_LOGD(node()->getLocation(), "READER_AGENT", "Execute agent %s - count : %i", ID_C_STR(node()->getId()), counter);
			for(auto neighbour : node()->outNeighbors()) {
				WriterAgent* neighbour_agent = static_cast<WriterAgent*>(neighbour->mutex().acquire().get());

				neighbour_agent->setCounter(neighbour_agent->getCounter()+1);
				neighbour->setWeight(random_weight(engine) * 10);

				neighbour->mutex().releaseAcquire();
			}
		}

		void setCounter(int count) {counter = count;}

		int getCounter() const {
			return counter;
		}
};

void to_json(nlohmann::json& j, const fpmas::api::utils::VirtualPtrWrapper<WriterAgent>& agent) {
	j["c"] = agent->getCounter();
}

void from_json(const nlohmann::json& j, fpmas::api::utils::VirtualPtrWrapper<WriterAgent>& agent) {
	WriterAgent* agent_ptr = new WriterAgent;
	agent_ptr->setCounter(j.at("c").get<int>());
	agent = {agent_ptr};
}

class HardSyncAgentModelIntegrationTest : public ::testing::Test {
	protected:
		static const unsigned int AGENT_BY_PROC = 50;
		static const unsigned int STEPS = 100;
		fpmas::model::AgentGraph<fpmas::synchro::HardSyncMode> agent_graph;
		fpmas::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};
};

class HardSyncReadersModelIntegrationTest : public HardSyncAgentModelIntegrationTest {
	protected:
		void SetUp() override {
			auto& group = model.buildGroup();
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
			scheduler.schedule(0, 1, group.job());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(HardSyncReadersModelIntegrationTest, test) {
	runtime.run(STEPS);
}

class HardSyncWritersModelIntegrationTest : public HardSyncAgentModelIntegrationTest {
	protected:
		void SetUp() override {
			auto& group = model.buildGroup();
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
			scheduler.schedule(0, 1, group.job());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(HardSyncWritersModelIntegrationTest, test) {
	runtime.run(STEPS);

	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		const WriterAgent* agent = static_cast<const WriterAgent*>(node.second->mutex().read().get());
		ASSERT_EQ(agent->getCounter(), STEPS * node.second->getIncomingEdges().size());
	}
}

class LinkerAgent : public fpmas::model::AgentBase<LinkerAgent, 3> {
	std::mt19937 engine;
	std::uniform_int_distribution<int> random_layer {0, 16};

	public:
	std::set<DistributedId> links;
	std::set<DistributedId> unlinks;

	void act() override {
		FPMAS_LOGD(node()->getLocation(), "GHOST_TEST", "Executing agent %s", ID_C_STR(node()->getId()));
		if(node()->getOutgoingEdges().size() >= 2) {
			auto out_neighbors = node()->outNeighbors();
			std::uniform_int_distribution<unsigned long> random_neighbor {0, out_neighbors.size()-1};
			auto src = out_neighbors[random_neighbor(engine)];
			auto tgt = out_neighbors[random_neighbor(engine)];
			if(src->getId() != tgt->getId()) {
				DistributedId current_edge_id = graph()->currentEdgeId();
				links.insert(current_edge_id++);
				graph()->link(src, tgt, random_layer(engine));
			}
		}
		if(node()->getOutgoingEdges().size() >= 1) {
			std::uniform_int_distribution<unsigned long> random_edge {0, node()->getOutgoingEdges().size()-1};
			auto index = random_edge(engine);
			auto edge = node()->getOutgoingEdges()[index];
			DistributedId id = edge->getId();
			unlinks.insert(id);
			graph()->unlink(edge);
		}
	}
};

void to_json(nlohmann::json& j, const fpmas::api::utils::VirtualPtrWrapper<LinkerAgent>& agent) {
	j["links"] = agent->links;
	j["unlinks"] = agent->unlinks;
}

void from_json(const nlohmann::json& j, fpmas::api::utils::VirtualPtrWrapper<LinkerAgent>& agent) {
	LinkerAgent* agent_ptr = new LinkerAgent;
	agent_ptr->links = j.at("links").get<std::set<DistributedId>>();
	agent_ptr->unlinks = j.at("unlinks").get<std::set<DistributedId>>();
	agent = {agent_ptr};
}

class ModelDynamicLinkGhostModeIntegrationTest : public ModelGhostModeIntegrationTest {
	protected:
		std::set<DistributedId> initial_links;

		void SetUp() override {
			auto& group = model.buildGroup();

			if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
			scheduler.schedule(0, 1, group.job());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(ModelDynamicLinkGhostModeIntegrationTest, test) {
	runtime.run(NUM_STEPS);

	fpmas::communication::TypedMpi<unsigned int> mpi {agent_graph.getMpiCommunicator()};

	// Build local links set
	std::set<DistributedId> links;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = static_cast<const LinkerAgent*>(node.second->mutex().read().get());
		links.insert(agent->links.begin(), agent->links.end());
	}

	// Build local unlinks set
	std::set<DistributedId> unlinks;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = static_cast<const LinkerAgent*>(node.second->mutex().read().get());
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

	if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
			auto& group = model.buildGroup();

			if(agent_graph.getMpiCommunicator().getRank() == 0) {
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
			scheduler.schedule(0, 1, group.job());
			scheduler.schedule(0, 10, model.loadBalancingJob());
		}
};

TEST_F(ModelDynamicLinkHardSyncModeIntegrationTest, test) {
	runtime.run(NUM_STEPS);

	fpmas::communication::TypedMpi<unsigned int> mpi {agent_graph.getMpiCommunicator()};

	// Build local links set
	std::set<DistributedId> links;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = static_cast<const LinkerAgent*>(node.second->mutex().read().get());
		links.insert(agent->links.begin(), agent->links.end());
	}

	// Build local unlinks set
	std::set<DistributedId> unlinks;
	for(auto node : agent_graph.getLocationManager().getLocalNodes()) {
		auto agent = static_cast<const LinkerAgent*>(node.second->mutex().read().get());
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

	if(agent_graph.getMpiCommunicator().getRank() == 0) {
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Initial link count : %lu",
				initial_links.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Link count : %lu - Unlink count : %lu",
				total_links.size(), total_unlinks.size());
		FPMAS_LOGI(agent_graph.getMpiCommunicator().getRank(), "HARD_SYNC_TEST", "Total edge count : %lu", edge_count);
	}
}
