#ifndef MACROS_H
#define MACROS_H

namespace FPMAS::graph::base {
	typedef int LayerId;
	static constexpr LayerId DefaultLayer = 0;
}

#define MPI_DISTRIBUTED_ID_TYPE \
	FPMAS::graph::parallel::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename, int> class S

#endif
