#ifndef FPMAS_TEST_AGENTS
#define FPMAS_TEST_AGENTS

#include "fpmas/model/environment.h"
#include "../mocks/model/mock_model.h"

using testing::Ge;

class ReaderAgent : public fpmas::model::AgentBase<ReaderAgent> {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight;
		int counter = 0;
	public:
		void act() override {
			FPMAS_LOGD(node()->location(), "READER_AGENT", "Execute agent %s - count : %i", FPMAS_C_STR(node()->getId()), counter);
			for(auto neighbor : node()->outNeighbors()) {
				ASSERT_THAT(dynamic_cast<const ReaderAgent*>(neighbor->mutex()->read().get())->getCounter(), Ge(counter));
			}
			counter++;
			node()->setWeight(random_weight(engine) * 10);
		}

		void setCounter(int count) {counter = count;}

		int getCounter() const {
			return counter;
		}

		static void to_json(nlohmann::json& j, const ReaderAgent* agent) {
			j["c"] = agent->getCounter();
		}

		static ReaderAgent* from_json(const nlohmann::json& j) {
			ReaderAgent* agent_ptr = new ReaderAgent;
			agent_ptr->setCounter(j.at("c").get<int>());
			return agent_ptr;
		}
};

class WriterAgent : public fpmas::model::AgentBase<WriterAgent> {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_weight;
		int counter = 0;
	public:
		void act() override {
			FPMAS_LOGD(node()->location(), "READER_AGENT", "Execute agent %s - count : %i", FPMAS_C_STR(node()->getId()), counter);
			for(auto neighbor : node()->outNeighbors()) {
				WriterAgent* neighbor_agent = dynamic_cast<WriterAgent*>(neighbor->mutex()->acquire().get());

				neighbor_agent->setCounter(neighbor_agent->getCounter()+1);
				neighbor->setWeight(random_weight(engine) * 10);

				neighbor->mutex()->releaseAcquire();
			}
		}

		void setCounter(int count) {counter = count;}

		int getCounter() const {
			return counter;
		}

		static void to_json(nlohmann::json& j, const WriterAgent* agent) {
			j["c"] = agent->getCounter();
		}

		static WriterAgent* from_json(const nlohmann::json& j) {
			WriterAgent* agent_ptr = new WriterAgent;
			agent_ptr->setCounter(j.at("c").get<int>());
			return agent_ptr;
		}
};

class LinkerAgent : public fpmas::model::AgentBase<LinkerAgent> {
	std::mt19937 engine;
	std::uniform_int_distribution<int> random_layer {0, 16};

	public:
	std::set<DistributedId> links;
	std::set<DistributedId> unlinks;

	void act() override {
		FPMAS_LOGD(node()->location(), "GHOST_TEST", "Executing agent %s", FPMAS_C_STR(node()->getId()));
		if(node()->getOutgoingEdges().size() >= 2) {
			auto out_neighbors = node()->outNeighbors();
			std::uniform_int_distribution<unsigned long> random_neighbor {0, out_neighbors.size()-1};
			auto src = out_neighbors[random_neighbor(engine)];
			auto tgt = out_neighbors[random_neighbor(engine)];
			if(src->getId() != tgt->getId()) {
				DistributedId current_edge_id = model()->graph().currentEdgeId();
				links.insert(current_edge_id++);
				model()->graph().link(src, tgt, random_layer(engine));
			}
		}
		if(node()->getOutgoingEdges().size() >= 1) {
			std::uniform_int_distribution<unsigned long> random_edge {0, node()->getOutgoingEdges().size()-1};
			auto index = random_edge(engine);
			auto edge = node()->getOutgoingEdges()[index];
			DistributedId id = edge->getId();
			unlinks.insert(id);
			model()->graph().unlink(edge);
		}
	}

	static void to_json(nlohmann::json& j, const LinkerAgent* agent) {
		j["links"] = agent->links;
		j["unlinks"] = agent->unlinks;
	}

	static LinkerAgent* from_json(const nlohmann::json& j) {
		LinkerAgent* agent_ptr = new LinkerAgent;
		agent_ptr->links = j.at("links").get<std::set<DistributedId>>();
		agent_ptr->unlinks = j.at("unlinks").get<std::set<DistributedId>>();
		return agent_ptr;
	}
};


class TestCell : public fpmas::model::Cell<TestCell> {
	public:
		int index;

		TestCell(int index) : index(index) {}

		static void to_json(nlohmann::json& j, const TestCell* cell) {
			j = cell->index;
		}
		static TestCell* from_json(const nlohmann::json& j) {
			return new TestCell(j.get<int>());
		}
};

class FakeRange : public fpmas::api::model::Range {
	public:
		unsigned int size() const override {
			return 1;
		}

		bool contains(fpmas::api::model::Cell* cell) const override {
			return true;
		}
};

class TestLocatedAgent : public fpmas::model::LocatedAgent<TestLocatedAgent> {
	private:
		FakeRange mobility_range;
		FakeRange perception_range;

	public:
		static const fpmas::model::Behavior<TestLocatedAgent> behavior;

		const fpmas::api::model::Range& mobilityRange() const override {
			return mobility_range;
		}

		const fpmas::api::model::Range& perceptionRange() const override {
			return perception_range;
		}

		void moveToNextCell() {
			std::unordered_map<int, TestCell*> cell_index;
			int current_index = static_cast<TestCell*>(this->location())->index;
			for(auto cell : outNeighbors<TestCell>(fpmas::api::model::MOVE)) {
				cell_index[cell->index] = cell;
			}

			int next_index = (current_index+1) % this->model()->graph().getMpiCommunicator().getSize();
			moveToCell(cell_index[next_index]);
		}
};

FPMAS_DEFAULT_JSON(DefaultMockAgentBase<1>)
FPMAS_DEFAULT_JSON(DefaultMockAgentBase<10>)
FPMAS_DEFAULT_JSON(TestLocatedAgent)

#define TEST_AGENTS ReaderAgent, WriterAgent, LinkerAgent,\
		DefaultMockAgentBase<1>, DefaultMockAgentBase<10>,\
		TestCell, TestLocatedAgent

FPMAS_JSON_SET_UP(TEST_AGENTS)

#endif
