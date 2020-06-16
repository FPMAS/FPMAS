#ifndef MACROS_H
#define MACROS_H

#include "api/utils/ptr_wrapper.h"
#include "api/model/model.h"

#define ID_C_STR(id) ((std::string) id).c_str()

#define MPI_DISTRIBUTED_ID_TYPE \
	FPMAS::graph::parallel::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename> class S

#define FPMAS_DEFAULT_JSON(AGENT) \
	namespace nlohmann {\
		template<>\
			struct adl_serializer<FPMAS::api::utils::VirtualPtrWrapper<AGENT>> {\
				static void to_json(json& j, const FPMAS::api::utils::VirtualPtrWrapper<AGENT>& data) {\
				}\
\
				static void from_json(const json& j, FPMAS::api::utils::VirtualPtrWrapper<AGENT>& ptr) {\
					ptr = FPMAS::api::utils::VirtualPtrWrapper<AGENT>(new AGENT);\
				}\
			};\
	}\

#endif
