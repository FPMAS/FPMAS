#include <iostream> 

#include "zoltan_cpp.h"
#include "communication/communication.h"

int main(int argc, char* argv[]) {
	MPI::Init(argc, argv);

	// Communication
	Communication communication;

	//Initialize the Zoltan library with a C language call
	float version;
	Zoltan_Initialize(argc, argv, &version);

	//Dynamically create Zoltan object.
	Zoltan *zz = new Zoltan(MPI_COMM_WORLD);
	zz->Set_Param("DEBUG_LEVEL", "0");
	zz->Set_Param("LB_METHOD", "GRAPH"); // Simulation parameter?
	zz->Set_Param("LB_APPROACH", "REPARTITION"); //REPARTITION //REFINE //PARTITION
	zz->Set_Param("NUM_GID_ENTRIES", "1");
	zz->Set_Param("NUM_LID_ENTRIES", "0");
	zz->Set_Param("OBJ_WEIGHT_DIM", "1");
	zz->Set_Param("EDGE_WEIGHT_DIM", "0");
	zz->Set_Param("RETURN_LISTS", "ALL");
	zz->Set_Param("CHECK_GRAPH", "0");
	zz->Set_Param("IMBALANCE_TOL", "1.02");


	//Several lines of code would follow, working with zz
	if(communication.rank() == 0) {
		std::cout << "" << std::endl;
		std::cout << "███████╗    ██████╗ ███╗   ███╗ █████╗ ███████╗" << std::endl;
		std::cout << "██╔════╝    ██╔══██╗████╗ ████║██╔══██╗██╔════╝" << std::endl;
		std::cout << "█████╗█████╗██████╔╝██╔████╔██║███████║███████╗" << std::endl;
		std::cout << "██╔══╝╚════╝██╔═══╝ ██║╚██╔╝██║██╔══██║╚════██║" << std::endl;
		std::cout << "██║         ██║     ██║ ╚═╝ ██║██║  ██║███████║" << std::endl;
		std::cout << "╚═╝         ╚═╝     ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝" << std::endl;
		std::cout << "Version 2.0" << std::endl;
		std::cout << "" << std::endl;
		std::cout << "-------------------------------------------------" << std::endl;
		//    std::cout << "- NB TimeStep      : " << sparam->getMax_timestep() << std::endl;
		std::cout << "- NB Proc          : " << communication.size() << std::endl;
		//  std::cout << "- NB Total Agent   : " << sparam->getTotal_agent()*communication->getSize() << std::endl;
		//std::cout << "- Optimisation     : " << sparam->getOptimisation() << std::endl;
		// if(sparam->getOptimisation() == 1) {
		//    std::cout << "- Ack period       : " << sparam->getPeriod_ack() << std::endl;
		//}
		//std::cout << "- Seed             : " << sparam->getSeed() << std::endl;
		std::cout << "-------------------------------------------------" << std::endl;
	}

	//Explicitly delete the Zoltan object
	delete zz;
	MPI::Finalize();
}
