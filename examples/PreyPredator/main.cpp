#include "serializer.h"
#include "prey_predator.h"

#define PREDATOR_COUNT 3

void init(int argc, char* argv[]);
int main(int argc, char* argv[]) {
	init(argc, argv);

	{
		DistributedGraph<Agent, HardSyncMode> dg;

		if(dg.getMpiCommunicator().getRank() == 0) {
			FPMAS_LOGI(0, "MAIN", "Building graph...");
			auto prey = dg.buildNode(0, Agent("Prey", PREY));

			for(int i = 0; i < PREDATOR_COUNT; i++) {
				auto predator = dg.buildNode(i+1, Agent("Predator " + std::to_string(i), PREDATOR));
				dg.link(predator, prey);
			}
		}
		FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Distributing graph...");
		dg.balance();
		FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Done...");

		for(auto node : dg.getNodes()) {
			FPMAS_LOGI(dg.getMpiCommunicator().getRank(), "MAIN", "Executing local agent %lu", node.first);
			// Acquiring the local agents is important, because the local
			// process might update the agent state, so other procs can't have
			// access to it while it's acting.
			node.second->data()->acquire()
				.act(node.second->outNeighbors()); // Process agent
			// Releases the current agent
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
