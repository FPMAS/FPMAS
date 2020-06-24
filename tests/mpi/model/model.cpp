#include "model/model.h"
#include "model/serializer.h"
#include "graph/parallel/distributed_graph.h"
#include "load_balancing/zoltan_load_balancing.h"
#include "synchro/ghost/ghost_mode.h"
#include "synchro/hard/hard_sync_mode.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"
#include "../mocks/model/mock_model.h"

#include <random>
#include <numeric>

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::InvokeWithoutArgs;

class ReaderAgent;
class WriterAgent;

FPMAS_JSON_SERIALIZE_AGENT(ReaderAgent, WriterAgent, MockAgentBase<1>, MockAgentBase<10>)
FPMAS_DEFAULT_JSON(MockAgentBase<1>)
FPMAS_DEFAULT_JSON(MockAgentBase<10>)

class ModelGhostModeIntegrationTest : public ::testing::Test {
	protected:
		inline static const int NODE_BY_PROC = 50;
		inline static const int NUM_STEPS = 100;
		fpmas::model::AgentGraph<fpmas::synchro::GhostMode> agent_graph;
		fpmas::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;

};

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

TEST_F(ModelGhostModeIntegrationExecutionTest, ghost_mode) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
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
		inline static const int NODE_BY_PROC = 50;
		inline static const int NUM_STEPS = 100;
		fpmas::model::AgentGraph<fpmas::synchro::HardSyncMode> agent_graph;
		fpmas::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};
		fpmas::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		fpmas::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, fpmas::api::model::TypeId> agent_types;

};

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
		inline static unsigned int AGENT_BY_PROC = 50;
		inline static unsigned int STEPS = 100;
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
		ASSERT_EQ(agent->getCounter(), STEPS * node.second->getIncomingArcs().size());
	}
}
