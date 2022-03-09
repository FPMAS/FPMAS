#include "fpmas.h"
#include "model/mock_model.h"
#include "gmock/gmock.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace testing;

class NeighborsTest : public Test {
	protected:
		static const std::size_t NUM_AGENTS {6};

		std::array<fpmas::model::AgentPtr, NUM_AGENTS> agents {{
			{new MockAgent<>}, {new MockAgent<>}, {new MockAgent<>},
				{new MockAgent<>}, {new MockAgent<>}, {new MockAgent<>}
		}};
		std::array<MockDistributedEdge<fpmas::api::model::AgentPtr, NiceMock>, NUM_AGENTS> edges ;
		std::vector<fpmas::model::Neighbor<MockAgent<>>> neighbors_list {
			{&agents[0], &edges[0]},
				{&agents[1], &edges[1]},
				{&agents[2], &edges[2]},
				{&agents[3], &edges[3]},
				{&agents[4], &edges[4]},
				{&agents[5], &edges[5]}
		};
		fpmas::model::Neighbors<MockAgent<>> neighbors;

		void SetUp() override {
			for(std::size_t i = 0; i < NUM_AGENTS; i++) {
				ON_CALL(*((MockAgent<>*) agents[i].get()), getField)
					.WillByDefault(Return(i));
			}
			reset_neighbors();
		}

		void reset_neighbors() {
			neighbors = {neighbors_list};
		}

};
const std::size_t NeighborsTest::NUM_AGENTS;

TEST_F(NeighborsTest, seed) {

	// Base seed
	std::vector<std::pair<fpmas::api::model::Agent*, fpmas::model::AgentEdge*>> items1;
	{
		fpmas::seed(0);
		neighbors.shuffle();
		for(auto neighbor : neighbors)
			items1.push_back({neighbor.agent(), neighbor.edge()});
	}

	// New seed
	std::vector<std::pair<fpmas::api::model::Agent*, fpmas::model::AgentEdge*>> items2;
	{
		fpmas::seed(1);
		reset_neighbors();
		neighbors.shuffle();
		for(auto neighbor : neighbors)
			items2.push_back({neighbor.agent(), neighbor.edge()});
	}

	// Shuffling must be different
	ASSERT_THAT(items1, Not(ElementsAreArray(items2)));

	// Get back to new seed
	std::vector<std::pair<fpmas::api::model::Agent*, fpmas::model::AgentEdge*>> items3;
	{
		fpmas::seed(0);
		reset_neighbors();
		neighbors.shuffle();
		for(auto neighbor : neighbors)
			items3.push_back({neighbor.agent(), neighbor.edge()});
	}

	// Test reproductibility
	ASSERT_THAT(items3, ElementsAreArray(items1));
}

TEST_F(NeighborsTest, shuffle) {
	// histogram[i][j] = how many times the agent i was set at position j
	std::array<std::array<std::size_t, NUM_AGENTS>, NUM_AGENTS> histogram;
	for(auto& agent : histogram)
		for(auto& position : agent)
			position = 0;

	for(std::size_t i = 0; i < 1000; i++) {
		neighbors.shuffle();
		std::set<fpmas::api::model::Agent*> agents_set;
		for(std::size_t j = 0; j < NUM_AGENTS; j++) {
			histogram[neighbors[j]->getField()][j]++;
			agents_set.insert(neighbors[j].agent());
		}
		ASSERT_THAT(agents_set, SizeIs(NUM_AGENTS));
	}

	for(auto agent : histogram)
		for(auto position : agent)
			ASSERT_NEAR((float) position / 1000, 1./NUM_AGENTS, 0.1);
}

TEST_F(NeighborsTest, filter) {
	neighbors.filter([] (const fpmas::model::Neighbor<MockAgent<>>& neighbor) {
			return neighbor.agent()->getField() > 3;}
			);
	std::vector<int> agent_fields;
	for(auto agent : neighbors)
		agent_fields.push_back(agent->getField());
	ASSERT_THAT(agent_fields, SizeIs(2));
	ASSERT_THAT(agent_fields, UnorderedElementsAre(4, 5));
}

bool operator<(const MockAgent<>& a1, const MockAgent<>& a2) {
	return a1.getField() < a2.getField();
}

TEST_F(NeighborsTest, sort_default_comparator) {
	neighbors.shuffle();
	neighbors.sort();

	int i = 0;
	for(auto neighbor : neighbors)
		ASSERT_EQ(neighbor.agent()->getField(), i++);
}

TEST_F(NeighborsTest, sort_custom_comparator) {
	neighbors.shuffle();
	neighbors.sort([] (const MockAgent<>& a1, const MockAgent<>& a2) {
			return a1.getField() > a2.getField();
			});

	int i = NUM_AGENTS-1;
	for(auto neighbor : neighbors)
		ASSERT_EQ(neighbor.agent()->getField(), i--);
}
