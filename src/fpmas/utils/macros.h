#ifndef FPMAS_MACROS_H
#define FPMAS_MACROS_H

/** \file src/fpmas/utils/macros.h
 * FPMAS macros.
 */

#define ID_C_STR(id) ((std::string) id).c_str()

#define MPI_DISTRIBUTED_ID_TYPE \
	fpmas::api::graph::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename> class S

#endif
