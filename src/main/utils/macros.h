#ifndef MACROS_H
#define MACROS_H

#define ID_C_STR(id) ((std::string) id).c_str()

#define MPI_DISTRIBUTED_ID_TYPE \
	FPMAS::graph::parallel::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename> class S

#endif
