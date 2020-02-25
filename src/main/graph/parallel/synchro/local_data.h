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
	template<class T> class LocalData : public SyncData<T> {
		public:
			LocalData();
			LocalData(const T&);
			LocalData(T&&);
	};

	/**
	 * Default constructor.
	 */
	template<class T> LocalData<T>::LocalData()
		: SyncData<T>() {}

	/**
	 * Builds a LocalData instance initialized with the specified data.
	 *
	 * @param data data to wrap
	 */
	template<class T> LocalData<T>::LocalData(T&& data)
		: SyncData<T>(std::move(data)) {}

	template<class T> LocalData<T>::LocalData(const T& data)
		: SyncData<T>(data) {}
}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::LocalData;

	/**
	 * Defines unserialization rules for LocalData.
	 */
    template <class T>
    struct adl_serializer<LocalData<T>> {
		/**
		 * Unserializes the provided json as a LocalData instance.
		 *
		 * This function is automatically called by the nlohmann library.
		 *
		 * @param j reference to the json instance to unserialize
		 * @return unserialized local data
		 */
		static LocalData<T> from_json(const json& j) {
			return LocalData<T>(j.get<T>());
		}
	};
}
#endif
