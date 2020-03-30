#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "utils/test.h"

#include "environment/grid_agent.h"
#include "environment/grid.h"

using FPMAS::environment::grid::Grid;
using FPMAS::environment::grid::GridAgent;
using FPMAS::environment::grid::VonNeumann;
using FPMAS::environment::grid::locationLayer;
using FPMAS::environment::grid::movableTo;
using FPMAS::agent::TypedPerception;

class GridAgentMock : public GridAgent<1, GhostMode, 1, GridAgentMock> {
	public:
		MOCK_METHOD(void, act, (node_ptr, env_type&), (override));
};

namespace nlohmann {
	template<>
    struct adl_serializer<GridAgentMock> {
		static void to_json(json& j, const GridAgentMock& data) {
			nlohmann::adl_serializer<GridAgent<1, GhostMode, 1, GridAgentMock>>
				::to_json(j, data);
		}

		static GridAgentMock from_json(const json& j) {
			return GridAgentMock();
		}

	};
}

class Mpi_AgentGridLocalMoveToTest : public ::testing::Test {
	protected:
		typedef Grid<VonNeumann, 1, GhostMode, 1, GridAgentMock> grid_type;
		grid_type grid {5, 1};
		grid_type::node_type* agentNode;
		void SetUp() override {
			if(grid.getMpiCommunicator().getRank() == 0) {
				agentNode = grid.buildNode(std::unique_ptr<grid_type::agent_type>(new GridAgentMock()));
				grid.link(
						agentNode->getId(),
						grid.id(1, 0),
						locationLayer(1, 1)
						);

				ASSERT_EQ(grid.getScheduler().get(0).size(), 1);
				ASSERT_EQ(grid.getScheduler().get(1).size(), 5);
			}
		}
};

TEST_F(Mpi_AgentGridLocalMoveToTest, manual_move_to) {
	if(grid.getMpiCommunicator().getRank() == 0) {
		const auto* nextCellNode = grid.getNode(grid.id(2, 0));
		dynamic_cast<GridAgentMock*>(agentNode->data()->read().get())->moveTo(
				agentNode,
				grid,
				TypedPerception<grid_type::node_type, movableTo(1, 1)>(nextCellNode)
				);

		ASSERT_EQ(const_cast<const grid_type::node_type*>(agentNode)->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 1);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(agentNode)->layer(locationLayer(1, 1)).getIncomingArcs().size(), 0);
		ASSERT_EQ(nextCellNode->layer(locationLayer(1, 1)).getIncomingArcs().size(), 1);
		ASSERT_EQ(nextCellNode->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 0);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(grid.getNode(grid.id(1, 0)))->layer(locationLayer(1, 1)).getIncomingArcs().size(), 0);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(grid.getNode(grid.id(1, 0)))->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 0);
	}
}

class Mpi_AgentGridLocalMoveToWithMovableToLinksTest : public Mpi_AgentGridLocalMoveToTest {
	protected:
		void SetUp() override {
			if(grid.getMpiCommunicator().getRank() == 0) {
				Mpi_AgentGridLocalMoveToTest::SetUp();
				grid.link(
						agentNode,
						grid.getNode(grid.id(0, 0)),
						movableTo(1, 1)
						);
				grid.link(
						agentNode,
						grid.getNode(grid.id(2, 0)),
						movableTo(1, 1)
						);
			}
		}
};

TEST_F(Mpi_AgentGridLocalMoveToWithMovableToLinksTest, perception_move_to) {
	if(grid.getMpiCommunicator().getRank() == 0) {
		auto* gridAgent = dynamic_cast<GridAgentMock*>(agentNode->data()->read().get());
		auto potentialDestinations = agentNode->data()->read()->perceptions<movableTo(1, 1)>(agentNode);
		ASSERT_EQ(potentialDestinations.get().size(), 2);

		auto destination = potentialDestinations.get<movableTo(1,1)>().at(1);
		gridAgent->moveTo(agentNode, grid, destination);

		ASSERT_EQ(const_cast<const grid_type::node_type*>(agentNode)->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 1);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(agentNode)->layer(locationLayer(1, 1)).getIncomingArcs().size(), 0);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(destination.node)->layer(locationLayer(1, 1)).getIncomingArcs().size(), 1);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(destination.node)->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 0);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(grid.getNode(grid.id(1, 0)))->layer(locationLayer(1, 1)).getIncomingArcs().size(), 0);
		ASSERT_EQ(const_cast<const grid_type::node_type*>(grid.getNode(grid.id(1, 0)))->layer(locationLayer(1, 1)).getOutgoingArcs().size(), 0);
	}
}

/*
 * In this test serie, we consider a grid of size {size-1, 1}, and a single
 * agent located on cell [0, 0]. The objective is to move the agent to [0, 1].
 * Depending on the location of agent, location and destination nodes,
 * different internal operations will occur, but the final result should always
 * be the same :
 * - location link from agent to [0, 0] is removed
 * - movableTo link from agent to [0, 1] is removed
 * - a location link from agent to [0, 1] is created.
 *
 * Those tests **only check the "agent side" of the moveTo operation**.
 * In consequence, the creation of "movableTo" and "perception" links, handled
 * by the Cells, are treated in a next test serie.
 */
class Mpi_AgentGridDistantMoveToTest : public ::testing::Test {
	protected:
		typedef Grid<VonNeumann, 1, GhostMode, 1, GridAgentMock> grid_type;
		grid_type* grid;

		// One node per proc
		std::unordered_map<DistributedId, int> partition1;

		// Agent goes on proc 0 (where its "location" is)
		std::unordered_map<DistributedId, int> partition2;

		// Agent goes on proc 1 (where its "destination" is)
		std::unordered_map<DistributedId, int> partition3;

		void SetUp() override {
			int size;
			MPI_Comm_size(MPI_COMM_WORLD, &size);
			grid = new grid_type(size-1, 1);
			if(size > 3) {
				if(grid->getMpiCommunicator().getRank() == 0) {
					auto agentNode = grid->buildNode(std::unique_ptr<grid_type::agent_type>(new GridAgentMock()));
					grid->link(
							agentNode,
							grid->getNode(grid->id(0, 0)),
							locationLayer(1, 1)
							);
					grid->link(
							agentNode,
							grid->getNode(grid->id(1, 0)),
							movableTo(1, 1)
							);
					for(auto node : grid->getNodes()) {
						partition1[node.first] = node.first.id();
					}

					partition2 = partition1;
					partition2[agentNode->getId()] = 0; // location of cell {0, 0}

					partition3 = partition1;
					partition3[agentNode->getId()] = 1; // location of cell {0, 1}
				}
			} else {
				PRINT_MIN_PROCS_WARNING(Mpi_AgentGridDistantMoveToTest, 4);
			}
		}

		void TearDown() override {
			delete grid;
		}

		void checkInitialLinks(int agentLocation) {
			int rank = grid->getMpiCommunicator().getRank();
			if(rank==0) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, 0}))->layer(locationLayer(1, 1)).getIncomingArcs().size(),
						1
						);
			} else if(rank==1) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, 1}))->layer(movableTo(1, 1)).getIncomingArcs().size(),
						1
						);
			} else if (rank == agentLocation) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, (unsigned int) rank}))->layer(locationLayer(1, 1)).getOutgoingArcs().size(),
						1
						);
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, (unsigned int) rank}))->layer(movableTo(1, 1)).getOutgoingArcs().size(),
						1
						);
			}
		}

		void checkFinalLinks(int agentLocation) {
			int rank = grid->getMpiCommunicator().getRank();
			if(rank==0) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, 0}))->layer(locationLayer(1, 1)).getIncomingArcs().size(),
						0
						);
			} else if(rank==1) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, 1}))->layer(movableTo(1, 1)).getIncomingArcs().size(),
						0
						);
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, 1}))->layer(locationLayer(1, 1)).getIncomingArcs().size(),
						1
						);
			} else if (rank == agentLocation) {
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, (unsigned int) rank}))->layer(locationLayer(1, 1)).getOutgoingArcs().size(),
						1
						);
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, (unsigned int) rank}))->layer(locationLayer(1, 1)).getOutgoingArcs().at(0)->getTargetNode()->getId(),
						DistributedId(0, 1)
						);
				ASSERT_EQ(
						const_cast<const grid_type::node_type*>(grid->getNode({0, (unsigned int) rank}))->layer(movableTo(1, 1)).getOutgoingArcs().size(),
						0
						);
			}
		}

		void moveAgent(int agentLocation) {
			if(grid->getMpiCommunicator().getRank() == agentLocation) {
				auto agentNode = grid->getNode({0, (unsigned int) grid->getMpiCommunicator().getSize()-1});
				auto* gridAgent = dynamic_cast<GridAgentMock*>(agentNode->data()->read().get());
				auto potentialDestinations = agentNode->data()->read()->perceptions<movableTo(1, 1)>(agentNode);
				ASSERT_EQ(potentialDestinations.get().size(), 1);

				auto destination = potentialDestinations.get<movableTo(1,1)>().at(0);
				gridAgent->moveTo(agentNode, *grid, destination);
			}
			grid->synchronize();
		}
};

/*
 * The Grid is distributed over [0, size-1[, and agent node is located at size-1.
 * In consequence, from the agent proc, "location" and "movableTo" nodes are
 * distant, and we move agent from "location" to "movableTo".
 */
TEST_F(Mpi_AgentGridDistantMoveToTest, move_with_distant_location_and_movableTo) {
	if(grid->getMpiCommunicator().getSize() > 3) {
		grid->distribute(partition1);
		ASSERT_EQ(grid->getNodes().size(), 1);

		int rank = grid->getMpiCommunicator().getRank();
		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 2);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 3);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 2);
		}

		checkInitialLinks(grid->getMpiCommunicator().getSize()-1);

		moveAgent(grid->getMpiCommunicator().getSize()-1);	

		checkFinalLinks(grid->getMpiCommunicator().getSize()-1);

		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 1);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 3);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 1);
		}
	}
}

/*
 * The Grid is distributed over [0, size-1[, and agent node is located at 0.
 * In consequence, from the agent proc, "location" is local and "movableTo" is 
 * distant, and we move agent from "location" to "movableTo".
 */
TEST_F(Mpi_AgentGridDistantMoveToTest, move_with_local_location_and_distant_movableTo) {
	if(grid->getMpiCommunicator().getSize() > 3) {
		grid->distribute(partition2);

		if(grid->getMpiCommunicator().getRank()==0) {
			ASSERT_EQ(grid->getNodes().size(), 2);
		} else if (grid->getMpiCommunicator().getRank() == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getNodes().size(), 0);
		} else {
			ASSERT_EQ(grid->getNodes().size(), 1);
		}

		int rank = grid->getMpiCommunicator().getRank();
		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 1);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 3);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 0);
		}

		checkInitialLinks(0);

		moveAgent(0);	

		checkFinalLinks(0);

		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 1);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 3);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 0);
		}
	}
}

/*
 * The Grid is distributed over [0, size-1[, and agent node is located at 1.
 * In consequence, from the agent proc, "location" is distant and "movableTo" is 
 * local, and we move agent from "location" to "movableTo".
 */
TEST_F(Mpi_AgentGridDistantMoveToTest, move_with_distant_location_and_local_movableTo) {
	if(grid->getMpiCommunicator().getSize() > 3) {
		grid->distribute(partition3);

		if(grid->getMpiCommunicator().getRank()==1) {
			ASSERT_EQ(grid->getNodes().size(), 2);
		} else if (grid->getMpiCommunicator().getRank() == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getNodes().size(), 0);
		} else {
			ASSERT_EQ(grid->getNodes().size(), 1);
		}

		int rank = grid->getMpiCommunicator().getRank();
		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 2);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 2);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 0);
		}

		checkInitialLinks(1);

		moveAgent(1);	

		checkFinalLinks(1);

		if(rank==0) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 1);
		} else if(rank==1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 2);
		} else if (rank == grid->getMpiCommunicator().getSize()-1) {
			ASSERT_EQ(grid->getGhost().getNodes().size(), 0);
		}
	}
}
