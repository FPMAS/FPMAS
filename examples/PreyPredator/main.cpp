#include "serializer.h"
#include "prey_predator.h"

#define PREDATOR_COUNT 5

void init(int argc, char* argv[]);
int main(int argc, char* argv[]) {
	init(argc, argv);

	{
		DistributedGraph<Agent, HardSyncData> dg;

		if(dg.getMpiCommunicator().getRank() == 0) {
			FPMAS_LOGI(0, "MAIN", "Building graph...");
			dg.buildNode(0, Agent("Prey", PREY));

			for(int i = 0; i < PREDATOR_COUNT; i++) {
				dg.buildNode(i+1, Agent("Predator " + std::to_string(i), PREDATOR));
				dg.link(i+1, 0, i);
			}
		}
		FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Distributing graph...");
		dg.distribute();
		FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Done...");

		for(auto node : dg.getNodes()) {
			FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Executing agent %lu", node.first);
			node.second->data()->acquire().act(
					node.second->outNeighbors()
					);
			node.second->data()->release();
		}
		dg.synchronize();
		for(auto node : dg.getNodes()) {
			json j = node.second->data();
			FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "RESULT", "%s", j.dump().c_str());
		}
	}

	MPI_Finalize();
}

void init(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	//Initialize the Zoltan library with a C language call
	float version;
	Zoltan_Initialize(argc, argv, &version);
}
