#include "gtest/gtest.h"

#include "utils/test.h"
#include "fake_grid_agent.h"

using FPMAS::test::ASSERT_CONTAINS;
using FPMAS::environment::grid::Grid;
using FPMAS::environment::grid::LOCATION;
using FPMAS::environment::grid::MOVABLE_TO;
using FPMAS::environment::grid::PERCEIVE;
using FPMAS::environment::grid::PERCEIVABLE_FROM;
using FPMAS::environment::grid::PERCEPTIONS;

/*
 * Test that ensures that the cell properly handle a single agent and updates
 * its MOVABLE_TO and PERCEIVABLE_FROM links
 */
class Mpi_LocalCellActNoPerception : public ::testing::Test {
	protected:
		typedef Grid<GhostMode, FakeGridAgent> grid_type;
		grid_type grid {5, 1};
		grid_type::node_type* agentNode;
		void SetUp() override {
			if(grid.getMpiCommunicator().getRank() == 0) {
				agentNode = grid.template buildAgent<FakeGridAgent>(1, 0);
				
				ASSERT_EQ(grid.getScheduler().get(0).size(), 1);
				ASSERT_EQ(grid.getScheduler().get(1).size(), 5);
			}
		}

		void checkAgentLinks() {
			auto perceivableFrom = agentNode->layer(PERCEIVABLE_FROM(1)).outNeighbors();
			ASSERT_EQ(perceivableFrom.size(), 2);
			ASSERT_CONTAINS(grid.getNode(grid.id(0, 0)), perceivableFrom);
			ASSERT_CONTAINS(grid.getNode(grid.id(2, 0)), perceivableFrom);

			auto movableTo = agentNode->layer(MOVABLE_TO).outNeighbors();
			ASSERT_EQ(movableTo.size(), 2);
			ASSERT_CONTAINS(grid.getNode(grid.id(0, 0)), movableTo);
			ASSERT_CONTAINS(grid.getNode(grid.id(2, 0)), movableTo);

			auto perceive = agentNode->layer(PERCEIVE).outNeighbors();
			ASSERT_EQ(perceive.size(), 2);
			ASSERT_CONTAINS(grid.getNode(grid.id(0, 0)), perceive);
			ASSERT_CONTAINS(grid.getNode(grid.id(2, 0)), perceive);

			ASSERT_EQ(grid.getNode(grid.id(1, 0))->layer(PERCEIVABLE_FROM(1)).getIncomingArcs().size(), 0);
			ASSERT_EQ(grid.getNode(grid.id(0, 0))->layer(PERCEIVABLE_FROM(1)).getIncomingArcs().size(), 1);
			ASSERT_EQ(grid.getNode(grid.id(2, 0))->layer(PERCEIVABLE_FROM(1)).getIncomingArcs().size(), 1);
		}
};

TEST_F(Mpi_LocalCellActNoPerception, cell_act) {
	if(grid.getMpiCommunicator().getRank() == 0) {
		auto cell = grid.getNode(grid.id(1, 0));

		cell->data()->acquire()->act(cell, grid);
		checkAgentLinks();

		// The agent is already updated, so nothing should change
		cell->data()->acquire()->act(cell, grid);
		checkAgentLinks();
	}
}

/*
 * Assuming that a Cell properly update MOVABLE_TO, PERCEIVE and PERCEIVABLE_FROM
 * relations (cf previous test), checks that agent perceptions are properly
 * updated.
 */
class Mpi_LocalCellActWithPerceptions : public Mpi_LocalCellActNoPerception {
	protected:
		grid_type::node_type* secondAgentNode;
		grid_type::node_type* thirdAgentNode;

		void SetUp() override {
			if(grid.getMpiCommunicator().getRank() == 0) {
				// Creates the first agent at [1, 0], same as before
				Mpi_LocalCellActNoPerception::SetUp();

				// Updates the perceptions and movableTo links of the first
				// agent
				auto cell = grid.getNode(grid.id(1, 0));
				cell->data()->acquire()->act(cell, grid);
				// Same as before, juste to be sure.
				checkAgentLinks();

				// Create a second agent at [4, 0], that have nothing to do
				// with others (never perceive or get perceived)
				secondAgentNode = grid.template buildAgent<FakeGridAgent>(4, 0);
				cell = grid.getNode(grid.id(4, 0));
				cell->data()->acquire()->act(cell, grid);

				// Checks that the first agent as not been altered
				checkAgentLinks();

				// Finally, creates a third agent at [2, 0], that must perceive
				// the first agent at [1, 0] and get perceived by it.
				thirdAgentNode = grid.template buildAgent<FakeGridAgent>(2, 0);
			}
		}
};

TEST_F(Mpi_LocalCellActWithPerceptions, cell_act) {
	if(grid.getMpiCommunicator().getRank() == 0) {
		auto cell = grid.getNode(grid.id(2, 0));
		// The first agent (at [1, 0]), should be PERCEIVABLE_FROM this cell
		ASSERT_EQ(cell->layer(PERCEIVABLE_FROM(1)).getIncomingArcs().size(), 1);
		// For now, the perceptions of this first agent have not been updated
		ASSERT_EQ(agentNode->layer(PERCEPTIONS).getOutgoingArcs().size(), 0);

		// Update agents linked to this cell
		cell->data()->acquire()->act(cell, grid);

		// The thirdAgent (at [2, 0]), should perceive the first agent
		ASSERT_EQ(thirdAgentNode->layer(PERCEPTIONS).outNeighbors().size(), 1);
		ASSERT_EQ(thirdAgentNode->layer(PERCEPTIONS).outNeighbors().at(0), agentNode);

		// The first agent should see the third agent
		ASSERT_EQ(agentNode->layer(PERCEPTIONS).outNeighbors().size(), 1);
		ASSERT_EQ(agentNode->layer(PERCEPTIONS).outNeighbors().at(0), thirdAgentNode);

		
		// The agent at [4, 0] should not perceive anything
		ASSERT_EQ(secondAgentNode->layer(PERCEPTIONS).outNeighbors().size(), 0);
	}
}

TEST_F(Mpi_LocalCellActWithPerceptions, cell_act_with_other_agent_on_the_cell) {
	if(grid.getMpiCommunicator().getRank() == 0) {
		// Updates the agent of the previous test
		auto cell = grid.getNode(grid.id(2, 0));
		cell->data()->acquire()->act(cell, grid);

		// From there, we are in the final situation of the previous test, that
		// should be correct
		
		// Now we add a last agent on the cell [2, 0]
		// This agent must perceive the same agents as the agent already
		// located there, PLUS this so-called agent
		auto lastAgentNode = grid.template buildAgent<FakeGridAgent>(2, 0);
		cell->data()->acquire()->act(cell, grid);

		ASSERT_EQ(lastAgentNode->layer(PERCEPTIONS).outNeighbors().size(), 2);
		ASSERT_CONTAINS(agentNode, lastAgentNode->layer(PERCEPTIONS).outNeighbors());
		ASSERT_CONTAINS(thirdAgentNode, lastAgentNode->layer(PERCEPTIONS).outNeighbors());

		ASSERT_EQ(thirdAgentNode->layer(PERCEPTIONS).outNeighbors().size(), 2);
		ASSERT_CONTAINS(agentNode, thirdAgentNode->layer(PERCEPTIONS).outNeighbors());
		ASSERT_CONTAINS(lastAgentNode, thirdAgentNode->layer(PERCEPTIONS).outNeighbors());

	}
}
