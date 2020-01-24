#include <iostream> 
#include <chrono>
#include <thread>

#include "zoltan_cpp.h"
#include <mpi.h>
#include "utils/config.h"

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/none.h"
#include "graph/parallel/synchro/ghost_data.h"
#include "graph/parallel/synchro/hard_sync_data.h"

#define PRINT_SPACE() if(dg.getMpiCommunicator().getRank() == 0){std::cout << std::endl;}else{std::this_thread::sleep_for(10ms);}

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::synchro::None;
using FPMAS::graph::parallel::synchro::GhostData;
using FPMAS::graph::parallel::synchro::HardSyncData;

#define SYNCHRO None
// #define SYNCHRO GhostData
// #define SYNCHRO HardSyncData

void init(int argc, char* argv[]);
void process(DistributedGraph<int, SYNCHRO>& dg);
void buildGraph(DistributedGraph<int, SYNCHRO>& dg);


int main(int argc, char* argv[]) {
	init(argc, argv);

	{
		DistributedGraph<int, SYNCHRO> dg;
		buildGraph(dg);

		process(dg);
	}

	MPI_Finalize();
}

void buildGraph(DistributedGraph<int, SYNCHRO>& dg) {
	if(dg.getMpiCommunicator().getRank() == 0) {
		for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
			dg.buildNode(i, 5);
		}
		for (int i = 0; i < dg.getMpiCommunicator().getSize(); ++i) {
			// Build a ring across the processors
			dg.link(i, (i+1) % dg.getMpiCommunicator().getSize(), i);
		}
	}

	dg.distribute();
	// dg.getGhost().synchronize();
}

using namespace std::chrono_literals;

void process(DistributedGraph<int, SYNCHRO>& dg) {
	int sum1 = 0;
	for(auto node : dg.getNodes()) {
		std::cout << "[" << dg.getMpiCommunicator().getRank() << "] local Node : " << node.first << std::endl;
		node.second->data()->acquire() = node.first;

		// Fake processing on odd nodes
		if(dg.getMpiCommunicator().getRank() % 2)
			std::this_thread::sleep_for(1s);

		for(auto arc : node.second->getOutgoingArcs()) {
			sum1 += arc->getTargetNode() // Node<SyncDataPtr<int>>
				->data() //SyncData<int>
				->read();
		}
	}
	dg.synchronize();

	PRINT_SPACE()
	std::cout << "[" << dg.getMpiCommunicator().getRank() << "] sum 1 : " << sum1 << std::endl;

	int sum2 = 0;
	for(auto node : dg.getNodes()) {
		for(auto arc : node.second->getOutgoingArcs()) {
			sum2 += arc->getTargetNode() // Node<SyncDataPtr<int>>
				->data() //SyncData<int>
				->read();
		}
	}
	dg.synchronize();

	PRINT_SPACE()
	std::cout << "[" << dg.getMpiCommunicator().getRank() << "] sum 2 : " << sum2 << std::endl;

}

void init(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	//Initialize the Zoltan library with a C language call
	float version;
	Zoltan_Initialize(argc, argv, &version);
}
