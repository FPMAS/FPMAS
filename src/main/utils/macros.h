#ifndef MACROS_H
#define MACROS_H

namespace FPMAS::graph::base {
	/**
	 * Type used to index layers.
	 */
	typedef int LayerId;

	/**
	 * Index of the layer used by default.
	 */
	static constexpr LayerId DefaultLayer = 0;
}

#define MPI_DISTRIBUTED_ID_TYPE \
	FPMAS::graph::parallel::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename, int> class S

#endif
