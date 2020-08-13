#ifndef DATA_UPDATE_PACK_H
#define DATA_UPDATE_PACK_H

#include "fpmas/api/graph/distributed_id.h"

namespace fpmas { namespace synchro {
	/**
	 * Structure used to send and receive data updates.
	 */
	template<typename T>
		struct DataUpdatePack {
			/**
			 * Associated node id.
			 */
			DistributedId id;
			/**
			 * Buffered data.
			 */
			T updated_data;

			/**
			 * DataUpdatePack constructor.
			 *
			 * @param id associated node id
			 * @param data reference to sent / received data
			 */
			DataUpdatePack(DistributedId id, const T& data)
				: id(id), updated_data(data) {}
		};
}}

namespace nlohmann {
	using fpmas::synchro::DataUpdatePack;

	/**
	 * DataUpdatePack JSON serialization.
	 */
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
