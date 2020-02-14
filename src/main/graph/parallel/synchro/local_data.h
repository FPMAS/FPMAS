#ifndef LOCAL_DATA_H
#define LOCAL_DATA_H

#include "sync_data.h"

namespace FPMAS::graph::parallel::synchro {

	/**
	 * Wrapper used for local nodes of a DistributedGraph.
	 *
	 * Wrapped data is truely local, and getters used are the defaults
	 * defined in SyncData.
	 */
	template<NODE_PARAMS> class LocalData : public SyncData<NODE_PARAMS_SPEC> {
		public:
			LocalData();
			LocalData(T);
	};

	/**
	 * Default constructor.
	 */
	template<NODE_PARAMS> LocalData<NODE_PARAMS_SPEC>::LocalData()
		: SyncData<NODE_PARAMS_SPEC>() {}

	/**
	 * Builds a LocalData instance initialized with the specified data.
	 *
	 * @param data data to wrap
	 */
	template<NODE_PARAMS> LocalData<NODE_PARAMS_SPEC>::LocalData(T data)
		: SyncData<NODE_PARAMS_SPEC>(data) {}
}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::LocalData;

	/**
	 * Defines unserialization rules for LocalData.
	 */
    template <NODE_PARAMS>
    struct adl_serializer<LocalData<NODE_PARAMS_SPEC>> {
		/**
		 * Unserializes the provided json as a LocalData instance.
		 *
		 * This function is automatically called by the nlohmann library.
		 *
		 * @param j reference to the json instance to unserialize
		 * @return unserialized local data
		 */
		static LocalData<NODE_PARAMS_SPEC> from_json(const json& j) {
			return LocalData<NODE_PARAMS_SPEC>(j.get<T>());
		}
	};
}
#endif
