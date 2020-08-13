#ifndef FPMAS_H
#define FPMAS_H

#include "mpi.h"
#include "zoltan_cpp.h"

#include "fpmas/communication/communication.h"

/** \namespace fpmas
 * Main `fpmas` namespace.
 */
/**
 * \namespace fpmas::api
 * Complete `fpmas` API.
 *
 * Users commonly use `fpmas` through the theoretical interfaces
 * (or "components") defined in this namespace.
 */

/** \namespace fpmas::api::communication
 * Communication API namespace.
 *
 * Contains features required to perform communications across processes.
 */
/** \namespace fpmas::communication
 * fpmas::api::communication implementations.
 */

/** \namespace fpmas::api::graph
 * Graph components API namespace.
 */
/** \namespace fpmas::graph
 * fpmas::api::graph implementations.
 */

/**\namespace fpmas::api::load_balancing
 * Load balancing API namespace.
 */
/**\namespace fpmas::load_balancing
 * fpmas::api::load_balancing implementations.
 */

/** \namespace fpmas::load_balancing::zoltan
 * Namespace containing definitions of all the required Zoltan query functions
 * to compute load balancing partitions based on the current graph nodes.
 */

/** \namespace fpmas::api::synchro
 * Defines any API related to synchronization among processes.
 */
/** \namespace fpmas::synchro
 * fpmas::api::synchro implementations.
 */
/** \namespace fpmas::synchro::hard
 * HardSyncMode implementation.
 */
/** \namespace fpmas::synchro::hard::api
 * Defines generic API used to handle HardSyncMode.
 */
/** \namespace fpmas::synchro::ghost
 * GhostMode implementation.
 */

/**
 * \namespace fpmas::api::scheduler
 *
 * Scheduler components API namespace.
 */
/**
 * \namespace fpmas::scheduler
 * fpmas::api::scheduler implementations.
 */

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
