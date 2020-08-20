#ifndef FPMAS_H
#define FPMAS_H

#include "fpmas/model/model.h"
#include "fpmas/model/serializer.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"

/** \file src/fpmas/fpmas.h
 * Main FPMAS include file.
 */

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

/**
 * \namespace fpmas::api::runtime
 *
 * Runtime components API namespace.
 */
/**
 * \namespace fpmas::runtime
 * fpmas::api::runtime implementations.
 */

/**
 * \namespace fpmas::api::model
 * Agent model API namespace.
 */
/**
 * \namespace fpmas::model
 * fpmas::api::model implementations.
 */

/**
 * \namespace fpmas::exceptions
 * FPMAS exceptions.
 */

/**
 * \namespace fpmas::api::utils
 * Utility classes used in the fpmas::api namespace.
 */
/**
 * \namespace fpmas::utils
 * Utility classes.
 */
/**
 * \namespace nlohmann
 * Namespace where JSON serialization rules can be defined.
 */

namespace fpmas {
	/**
	 * Initializes FPMAS.
	 */
	void init(int argc, char** argv) {
		MPI_Init(&argc, &argv);
		float v;
		Zoltan_Initialize(argc, argv, &v);
		fpmas::api::communication::createMpiTypes();
	}

	/**
	 * Finalizes FPMAS.
	 *
	 * No FPMAS calls are assumed to be performed after this call.
	 * Moreover, notice that because this function calls `MPI_Finalize`, all
	 * FPMAS related structures must have been destroyed to avoid unexpected
	 * behaviors. This can be easily achieved using _blocks_.
	 *
	 * \par Example
	 *
	 * ```cpp
	 * #include "fpmas.h"
	 *
	 * ...
	 *
	 * int main(int argc, char** argv) {
	 * 	// Initializes MPI and FPMAS related structures
	 * 	fpmas::init(argc, argv);
	 *
	 * 	{
	 * 		// Beginning of Model scope
	 * 		fpmas::model::DefaultModel model;
	 *
	 * 		...
	 *
	 * 		// End of Model scope : model is destroyed
	 * 	}
	 * 	fpmas::finalize();
	 * }
	 * ```
	 */
	void finalize() {
		fpmas::api::communication::freeMpiTypes();
		MPI_Finalize();
	}
}
#endif
