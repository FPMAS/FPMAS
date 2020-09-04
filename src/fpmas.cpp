#include "fpmas.h"

namespace fpmas {
	void init(int argc, char** argv) {
		MPI_Init(&argc, &argv);
		float v;
		Zoltan_Initialize(argc, argv, &v);
		fpmas::api::communication::createMpiTypes();
	}

	void finalize() {
		fpmas::api::communication::freeMpiTypes();
		MPI_Finalize();
	}
}
