#ifndef FPMAS_DATA_UPDATE_PACK_H
#define FPMAS_DATA_UPDATE_PACK_H

/** \file src/fpmas/utils/data_update_pack.h
 * DataUpdatePack implementation.
 */

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

	/**
	 * Structure used to send and receive node updates, including data and
	 * weight.
	 */
	template<typename T>
		struct NodeUpdatePack : public DataUpdatePack<T> {
			/**
			 * Updated weight.
			 */
			float updated_weight;

			/**
			 * NodeUpdatePack constructor.
			 *
			 * @param id associated node id
			 * @param data reference to sent / received data
			 * @param weight updated weight
			 */
			NodeUpdatePack(DistributedId id, const T& data, float weight)
				: DataUpdatePack<T>(id, data), updated_weight(weight) {}
		};
}}

namespace nlohmann {
	using fpmas::synchro::DataUpdatePack;
	using fpmas::synchro::NodeUpdatePack;

	/**
	 * DataUpdatePack JSON serialization.
	 */
    template <typename T>
    struct adl_serializer<DataUpdatePack<T>> {
		/**
		 * DataUpdatePack json unserialization.
		 *
		 * @param j json
		 * @return unserialized DataUpdatePack
		 */
        static DataUpdatePack<T> from_json(const json& j) {
            return {j[0].get<DistributedId>(), j[1].get<T>()};
        }

		/**
		 * DataUpdatePack json serialization.
		 *
		 * @param j json
		 * @param data DataUpdatePack to serialize
		 */
        static void to_json(json& j, const DataUpdatePack<T>& data) {
            j = json::array({data.id, data.updated_data});
        }
    };

	/**
	 * DataUpdatePack JSON serialization.
	 */
    template <typename T>
    struct adl_serializer<NodeUpdatePack<T>> {
		/**
		 * NodeUpdatePack json unserialization.
		 *
		 * @param j json
		 * @return unserialized NodeUpdatePack
		 */
        static NodeUpdatePack<T> from_json(const json& j) {
            return {j[0].get<DistributedId>(), j[1].get<T>(), j[2].get<float>()};
        }
		/**
		 * NodeUpdatePack json serialization.
		 *
		 * @param j json
		 * @param data NodeUpdatePack to serialize
		 */
        static void to_json(json& j, const NodeUpdatePack<T>& data) {
            j = json::array({data.id, data.updated_data, data.updated_weight});
        }
    };
}
#endif
