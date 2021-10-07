#include "fpmas.h"

namespace fpmas {
	static bool seed_called = false;

	void init(int argc, char** argv) {
		MPI_Init(&argc, &argv);
		float v;
		Zoltan_Initialize(argc, argv, &v);
		fpmas::api::communication::createMpiTypes();
		fpmas::communication::WORLD.init();
		// Seeds only if fpmas::seed() was not called before fpmas::init().
		// If fpmas::seed() is called after fpmas::init(), this has no effect.
		if(!seed_called)
			seed(default_seed);
	}

	void finalize() {
		fpmas::api::communication::freeMpiTypes();
		MPI_Finalize();
	}

	void seed(unsigned long seed) {
		std::mt19937 random_seed(seed);
		runtime::Runtime::seed(random_seed());
		model::RandomNeighbors::seed(random_seed());
		model::RandomMapping::seed(random_seed());
		random::rd_choices.seed(random_seed());

		seed_called = true;
	}
}
