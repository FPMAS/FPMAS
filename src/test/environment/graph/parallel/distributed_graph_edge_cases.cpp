#include "gtest/gtest.h"
#include "environment/graph/parallel/distributed_graph.h"
#include "test_utils/test_utils.h"

TEST(Mpi_DistributedGraphEdgeCases, duplicate_imported_arc_bug) {
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(size < 3){
		PRINT_MIN_PROCS_WARNING(duplicate_imported_arc_bug, 3)
			return;
	}

	int data = 1;
	if(rank == 0 || rank == 1 || rank == 2) {
		DistributedGraph<int> dg = DistributedGraph<int>({0, 1, 2});
		if(rank == 0) {
			dg.buildNode(0ul, &data);
			dg.getGhost()->buildNode(1ul);
			dg.getProxy()->setOrigin(1ul, 0);
			dg.getProxy()->setCurrentLocation(1ul, 1);
			dg.getGhost()->link(dg.getNodes().at(0ul), dg.getGhost()->getNodes().at(1ul), 0ul);

			dg.buildNode(2ul, &data);
			dg.getNodes().at(2ul)->setWeight(10.);
		}
		else if(rank == 1) {
			dg.buildNode(1ul, &data);
			dg.getGhost()->buildNode(0ul);
			dg.getProxy()->setOrigin(0ul, 0);
			dg.getProxy()->setCurrentLocation(0ul, 0);
			dg.getGhost()->link(dg.getGhost()->getNodes().at(0ul), dg.getNodes().at(1ul), 0ul);

			dg.buildNode(3ul, &data);
			dg.getNodes().at(3ul)->setWeight(10.);
		}

		dg.distribute();
		dg.getProxy()->synchronize();

		if(rank == 0) {
			dg.getNodes().begin()->second->setWeight(1.);
		}
		else if(rank == 1) {
			dg.getNodes().begin()->second->setWeight(1.);
		}
		else if(rank == 2) {
			ASSERT_EQ(dg.getNodes().size(), 2);
			for(auto node : dg.getNodes()) {
				std::cout << "n:" << node.first << std::endl;
				for(auto in : node.second->getIncomingArcs())
					std::cout << "in:" << in->getId() << std::endl;
				for(auto out : node.second->getOutgoingArcs())
					std::cout << "out:" << out->getId() << std::endl;
			}
			dg.getGhost()->clear(dg.removeNode(0ul));
		}
		dg.distribute();
	}
	else {
		DistributedGraph<int> dg = DistributedGraph<int>({rank});
	}

}
