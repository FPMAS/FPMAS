#ifndef MACROS_H
#define MACROS_H

#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/api/model/model.h"

#define ID_C_STR(id) ((std::string) id).c_str()

#define MPI_DISTRIBUTED_ID_TYPE \
	fpmas::graph::parallel::DistributedId::mpiDistributedIdType

#define SYNC_MODE template<typename> class S

#define FPMAS_DEFAULT_JSON(AGENT) \
	namespace nlohmann {\
		template<>\
			struct adl_serializer<fpmas::api::utils::VirtualPtrWrapper<AGENT>> {\
				static void to_json(json& j, const fpmas::api::utils::VirtualPtrWrapper<AGENT>& data) {\
				}\
\
				static void from_json(const json& j, fpmas::api::utils::VirtualPtrWrapper<AGENT>& ptr) {\
					ptr = fpmas::api::utils::VirtualPtrWrapper<AGENT>(new AGENT);\
				}\
			};\
	}\

#endif
