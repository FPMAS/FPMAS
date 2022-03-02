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
			seed(random::default_seed);
	}

	void finalize() {
		fpmas::api::communication::freeMpiTypes();
		MPI_Finalize();
	}

	void seed(unsigned long seed) {
		// Stores the seed
		fpmas::random::seed = seed;


		// Deterministic, but ensures all predefined random generators get a
		// different seed
		fpmas::random::mt19937 random_seed(seed);

		// Since seed() must be called from all processes and random_seed has
		// just been seeded, random_seed() is guaranteed to produce the same
		// results on all processes.
		runtime::Runtime::seed(random_seed());
		model::RandomNeighbors::seed(random_seed());
		model::RandomMapping::seed(random_seed());
		random::rd_choices.seed(random_seed());

		seed_called = true;
	}
}
