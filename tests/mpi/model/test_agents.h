#ifndef FPMAS_TEST_AGENTS_H
#define FPMAS_TEST_AGENTS_H

#include "fpmas/model/spatial/spatial_model.h"
#include "fpmas/model/spatial/grid.h"
#include "fpmas/model/spatial/graph.h"
#include "fpmas/model/spatial/von_neumann.h"
#include "../../mocks/model/mock_model.h"

#define TEST_AGENTS BasicAgent, ReaderAgent, WriterAgent, LinkerAgent,\
		DefaultMockAgentBase<1>, DefaultMockAgentBase<10>,\
		TestCell, TestSpatialAgent::JsonBase, fpmas::model::GridCell::JsonBase,\
		TestGridAgent::JsonBase,\
		fpmas::model::GraphCell::JsonBase

using testing::Ge;

class BasicAgent : public fpmas::model::AgentBase<BasicAgent> {
};

FPMAS_DEFAULT_JSON(BasicAgent);
FPMAS_DEFAULT_DATAPACK(BasicAgent);


class ReaderWriterBase {
	protected:
		fpmas::random::mt19937_64 rd_engine;
		int counter = 0;

	public:
		void setCounter(int count) {counter = count;}

		int getCounter() const {
			return counter;
		}

		void hello() {};
};

class ReaderAgent : public ReaderWriterBase, public fpmas::model::AgentBase<ReaderAgent> {
	private:
		std::uniform_real_distribution<float> random_weight;
	public:
		void read() {
			FPMAS_LOGD(
					node()->location(), "READER_AGENT",
					"Execute agent %s - count : %i",
					FPMAS_C_STR(node()->getId()), counter);
			for(auto neighbor_node : node()->outNeighbors()) {
				const auto* agent = neighbor_node->mutex()->read().get();
				if(const ReaderAgent* neighbor = dynamic_cast<const ReaderAgent*>(agent)) {
					FPMAS_LOGD(
							node()->location(), "READER_AGENT",
							"Reading neighbor %s - count : %i",
							FPMAS_C_STR(neighbor->node()->getId()),
							neighbor->getCounter());
					ASSERT_THAT(neighbor->getCounter(), Ge(this->model()->runtime().currentDate()));
				}
				neighbor_node->mutex()->releaseRead();
			}
			counter++;
			node()->setWeight(random_weight(rd_engine) * 10);
		}

		static void to_json(nlohmann::json& j, const ReaderAgent* agent) {
			j["c"] = agent->getCounter();
			j["rd"] = agent->rd_engine;
		}

		static ReaderAgent* from_json(const nlohmann::json& j) {
			ReaderAgent* agent_ptr = new ReaderAgent;
			agent_ptr->setCounter(j.at("c").get<int>());
			agent_ptr->rd_engine = j.at("rd").get<decltype(rd_engine)>();
			return agent_ptr;
		}

		static std::size_t size(
				const fpmas::io::datapack::ObjectPack& p, const ReaderAgent* agent) {
			return p.size<int>() + p.size(agent->rd_engine);
		}

		static void to_datapack(
				fpmas::io::datapack::ObjectPack& p, const ReaderAgent* agent) {
			p.put(agent->getCounter());
			p.put(agent->rd_engine);
		}

		static ReaderAgent* from_datapack(const fpmas::io::datapack::ObjectPack& p) {
			ReaderAgent* agent_ptr = new ReaderAgent;
			agent_ptr->setCounter(p.get<int>());
			agent_ptr->rd_engine = p.get<decltype(rd_engine)>();
			return agent_ptr;
		}
};

class WriterAgent : public ReaderWriterBase, public fpmas::model::AgentBase<WriterAgent> {
	private:
		std::uniform_real_distribution<float> random_weight;
	public:
		void write() {
			FPMAS_LOGD(node()->location(), "WRITER_AGENT",
					"Execute agent %s - count : %i",
					FPMAS_C_STR(node()->getId()), counter);
			for(auto neighbor : node()->outNeighbors()) {
				ReaderWriterBase* neighbor_agent
					= dynamic_cast<ReaderWriterBase*>(neighbor->mutex()->acquire().get());

				neighbor_agent->setCounter(neighbor_agent->getCounter()+1);
				neighbor->setWeight(random_weight(rd_engine) * 10);

				neighbor->mutex()->releaseAcquire();
			}
		}

		static void to_json(nlohmann::json& j, const WriterAgent* agent) {
			j["c"] = agent->getCounter();
			j["rd"] = agent->rd_engine;
		}

		static WriterAgent* from_json(const nlohmann::json& j) {
			WriterAgent* agent_ptr = new WriterAgent;
			agent_ptr->setCounter(j.at("c").get<int>());
			agent_ptr->rd_engine = j.at("rd").get<decltype(rd_engine)>();
			return agent_ptr;
		}

		static std::size_t size(
				const fpmas::io::datapack::ObjectPack& p, const WriterAgent* agent) {
			return p.size<int>() + p.size(agent->rd_engine);
		}

		static void to_datapack(
				fpmas::io::datapack::ObjectPack& p, const WriterAgent* agent) {
			p.put(agent->getCounter());
			p.put(agent->rd_engine);
		}

		static WriterAgent* from_datapack(const fpmas::io::datapack::ObjectPack& p) {
			WriterAgent* agent_ptr = new WriterAgent;
			agent_ptr->setCounter(p.get<int>());
			agent_ptr->rd_engine = p.get<decltype(rd_engine)>();
			return agent_ptr;
		}

};

class LinkerAgent : public fpmas::model::AgentBase<LinkerAgent> {
	std::mt19937 engine;
	std::uniform_int_distribution<int> random_layer {0, 16};

	public:
	std::set<DistributedId> links;
	std::set<DistributedId> unlinks;

	void link() {
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

	static std::size_t size(const fpmas::io::datapack::ObjectPack& p, const LinkerAgent* agent) {
		return p.size(agent->links) + p.size(agent->unlinks);
	}
	static void to_datapack(fpmas::io::datapack::ObjectPack& p, const LinkerAgent* agent) {
		p.put(agent->links);
		p.put(agent->unlinks);
	}

	static LinkerAgent* from_datapack(const fpmas::io::datapack::ObjectPack& p) {
		LinkerAgent* agent_ptr = new LinkerAgent;
		agent_ptr->links = p.get<std::set<DistributedId>>();
		agent_ptr->unlinks = p.get<std::set<DistributedId>>();
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

		static std::size_t size(const fpmas::io::datapack::ObjectPack& p, const TestCell* cell) {
			return p.size(cell->index);
		}

		static void to_datapack(fpmas::io::datapack::ObjectPack& p, const TestCell* cell) {
			p.put(cell->index);
		}
		static TestCell* from_datapack(const fpmas::io::datapack::ObjectPack& p) {
			return new TestCell(p.get<int>());
		}
};

namespace fpmas { namespace io { namespace json {
	template<>
		struct light_serializer<fpmas::api::utils::PtrWrapper<TestCell>> {
			static void to_json(light_json&, const fpmas::api::utils::PtrWrapper<TestCell>&) {
			}

			static void from_json(const light_json&, fpmas::api::utils::PtrWrapper<TestCell>& data) {
				data = {new TestCell(0)};
			}


		};
}}}

namespace fpmas { namespace io { namespace datapack {
	template<>
		struct LightSerializer<fpmas::api::utils::PtrWrapper<TestCell>> {
			static std::size_t size(const LightObjectPack&, const fpmas::api::utils::PtrWrapper<TestCell>&) {
				return 0;
			}

			static void to_datapack(LightObjectPack&, const fpmas::api::utils::PtrWrapper<TestCell>&) {
			}
			static fpmas::api::utils::PtrWrapper<TestCell> from_datapack(const LightObjectPack&) {
				return {new TestCell(0)};
			}
		};

}}}

class FakeRange : public fpmas::api::model::Range<TestCell> {
	private:
		unsigned int size;
		unsigned int num_cells_in_ring;
	public:
		FakeRange(unsigned int size)
			: size(size) {}

		FakeRange(unsigned int size, unsigned int num_cells_in_ring)
			: size(size), num_cells_in_ring(num_cells_in_ring) {}

		bool contains(TestCell* root, TestCell* cell) const override {
			int root_index = root->index;
			int cell_index = cell->index;
			unsigned int distance = std::abs(cell_index - root_index);
			if(distance > num_cells_in_ring / 2)
				distance = num_cells_in_ring-distance;
			if(distance <= size)
				return true;
			return false;
		}

		std::size_t radius(TestCell*) const override {
			return size;
		}
};

class TestSpatialAgent : public fpmas::model::SpatialAgent<TestSpatialAgent, TestCell> {
	private:
		unsigned int range_size;
		unsigned int num_cells_in_ring;
		FakeRange mobility_range;
		FakeRange perception_range;

	public:
		static const fpmas::model::Behavior<TestSpatialAgent> behavior;

		TestSpatialAgent(unsigned int range_size, unsigned int num_cells_in_ring) : 
				range_size(range_size), num_cells_in_ring(num_cells_in_ring),
			mobility_range(range_size, num_cells_in_ring),
			perception_range(range_size, num_cells_in_ring) {}

		FPMAS_MOBILITY_RANGE(mobility_range);
		FPMAS_PERCEPTION_RANGE(perception_range);

		/**
		 * Normally not publicly accessible, but made public for test purpose.
		 */
		TestCell* testLocation() {
			return this->locationCell();
		}

		void moveToNextCell() {
			std::unordered_map<int, TestCell*> cell_index;
			int current_index = this->locationCell()->index;
			for(auto cell : outNeighbors<TestCell>(fpmas::api::model::MOVE)) {
				cell_index[cell->index] = cell;
			}

			int next_index = (current_index+1) % this->model()->graph().getMpiCommunicator().getSize();

			FPMAS_LOGI(
					this->model()->getMpiCommunicator().getRank(),
					"TEST_MOVE_AGENT", "%s moves to %s",
					FPMAS_C_STR(this->node()->getId()),
					FPMAS_C_STR(cell_index[next_index]->node()->getId())
					);
			moveTo(cell_index[next_index]);
		}

		static void to_json(nlohmann::json& j, const TestSpatialAgent* agent) {
			j = {{"r", agent->range_size}, {"n", agent->num_cells_in_ring}};
		}

		static TestSpatialAgent* from_json(const nlohmann::json& j) {
			return new TestSpatialAgent(
					j.at("r").get<unsigned int>(),
					j.at("n").get<unsigned int>()
					);
		}

		static std::size_t size(const fpmas::io::datapack::ObjectPack& p, const TestSpatialAgent*) {
			return 2*p.size<unsigned int>();
		}

		static void to_datapack(fpmas::io::datapack::ObjectPack& p, const TestSpatialAgent* agent) {
			p.put(agent->range_size);
			p.put(agent->num_cells_in_ring);
		}

		static TestSpatialAgent* from_datapack(const fpmas::io::datapack::ObjectPack& p) {
			unsigned int range_size = p.get<unsigned int>();
			unsigned int num_cells_in_ring = p.get<unsigned int>();
			return new TestSpatialAgent(range_size, num_cells_in_ring);
		}
};

class TestGridAgent : public fpmas::model::GridAgent<TestGridAgent> {
	static fpmas::model::VonNeumannRange<fpmas::model::VonNeumannGrid<>> range;

	public:
	FPMAS_MOBILITY_RANGE(range);
	FPMAS_PERCEPTION_RANGE(range);

	void move() {
		this->moveTo(this->mobilityField().random());
	}
};

FPMAS_DEFAULT_JSON(DefaultMockAgentBase<1>);
FPMAS_DEFAULT_JSON(DefaultMockAgentBase<10>);
FPMAS_DEFAULT_JSON(TestGridAgent);
FPMAS_DEFAULT_DATAPACK(DefaultMockAgentBase<1>);
FPMAS_DEFAULT_DATAPACK(DefaultMockAgentBase<10>);
FPMAS_DEFAULT_DATAPACK(TestGridAgent);
#endif
