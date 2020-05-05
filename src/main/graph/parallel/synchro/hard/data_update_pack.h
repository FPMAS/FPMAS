#ifndef DATA_UPDATE_PACK_H
#define DATA_UPDATE_PACK_H

#include "graph/parallel/distributed_id.h"

namespace FPMAS::graph::parallel::synchro::hard {
	template<typename T>
		struct DataUpdatePack {
			DistributedId id;
			T updatedData;

			DataUpdatePack(DistributedId id, const T& data)
				: id(id), updatedData(data) {}
		};

}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;
    template <typename T>
    struct adl_serializer<DataUpdatePack<T>> {
        static DataUpdatePack<T> from_json(const json& j) {
			std::cout << "from : " << j.dump() << std::endl;
            return {j[0].get<DistributedId>(), j[1].get<T>()};
        }

        static void to_json(json& j, const DataUpdatePack<T>& data) {
			std::cout << "to : " << j.dump() << std::endl;
            j = json::array({data.id, data.updatedData});
        }
    };
}
#endif
