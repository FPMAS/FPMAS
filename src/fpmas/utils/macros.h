#ifndef MACROS_H
#define MACROS_H

#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/api/model/model.h"

#define ID_C_STR(id) ((std::string) id).c_str()

#define MPI_DISTRIBUTED_ID_TYPE \
	fpmas::api::graph::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename> class S

#endif
