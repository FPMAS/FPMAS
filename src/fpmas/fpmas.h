#ifndef FPMAS_H
#define FPMAS_H

#include "mpi.h"
#include "zoltan_cpp.h"

#include "fpmas/communication/communication.h"

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
#endif
