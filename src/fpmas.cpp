#include "fpmas.h"

namespace fpmas {
	void init(int argc, char** argv) {
		MPI_Init(&argc, &argv);
		float v;
		Zoltan_Initialize(argc, argv, &v);
		fpmas::api::communication::createMpiTypes();
		fpmas::communication::WORLD.init();
	}

	void finalize() {
		fpmas::api::communication::freeMpiTypes();
		MPI_Finalize();
	}

	void seed(unsigned long seed) {
		std::mt19937 random_seed(seed);
		runtime::Runtime::seed(random_seed());
		model::RandomNeighbors::seed(random_seed());
	}
}
