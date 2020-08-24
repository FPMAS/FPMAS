#ifndef FPMAS_H
#define FPMAS_H

#include "fpmas/model/model.h"
#include "fpmas/model/serializer.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"

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

/** \mainpage FPMAS API reference
 *
 * \section what What is FPMAS?
 *
 * FPMAS is an HPC (High Performance Computing) Multi-Agent simulation platform
 * designed to run on massively parallel and distributed memory environments,
 * such as computing clusters.
 *
 * The main advantage of FPMAS is that it is designed to abstract as much as
 * possible MPI and parallel features to the final user. Concretely, no
 * parallel or distributed computing skills are required to run Multi-Agent
 * simulations on hundreds of processor cores.
 *
 * Moreover, FPMAS offers powerful features to allow **concurrent write
 * operations accross distant processors**. This allows users to easily write
 * general purpose code without struggling with distributed computing issues,
 * while still allowing an implicit large scale distributed execution.
 *
 * FPMAS also automatically handles load balancing accross processors to
 * distribute the user defined Multi-Agent model on the available computing
 * resources.
 *
 * The underlying synchronized distributed graph structure used to represent
 * Multi-Agent models might also be used in applications that are not related
 * to Multi-Agent Systems to take advantage of other features such as load
 * balancing or distant write operations.
 *
 * \image html mior_dist.png "Simple model automatic distribution example on 4 cores"
 */

/** \file src/fpmas.h
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

#endif
