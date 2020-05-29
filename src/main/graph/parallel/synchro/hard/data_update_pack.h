#ifndef DATA_UPDATE_PACK_H
#define DATA_UPDATE_PACK_H

#include "graph/parallel/distributed_id.h"

namespace FPMAS::graph::parallel::synchro::hard {
	template<typename T>
		struct DataUpdatePack {
			DistributedId id;
			T updated_data;

			DataUpdatePack(DistributedId id, const T& data)
				: id(id), updated_data(data) {}
		};

}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;
    template <typename T>
    struct adl_serializer<DataUpdatePack<T>> {
        static DataUpdatePack<T> from_json(const json& j) {
            return {j[0].get<DistributedId>(), j[1].get<T>()};
        }

        static void to_json(json& j, const DataUpdatePack<T>& data) {
            j = json::array({data.id, data.updated_data});
        }
    };
}
#endif
