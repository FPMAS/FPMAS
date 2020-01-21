#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace FPMAS {
	namespace graph {

		template <class T> class SyncData {
			protected:
				T data;
			public:
				SyncData();
				SyncData(T);
				virtual T& get();
				virtual const T& get() const;

		};

		template<class T> SyncData<T>::SyncData() {
		}

		template<class T> SyncData<T>::SyncData(T data) : data(data) {
		}

		template<class T> T& SyncData<T>::get() {
			return data;
		}

		template<class T> const T& SyncData<T>::get() const {
			return data;
		}

		template<class T> class LocalData : public SyncData<T> {
			public:
				LocalData();
				LocalData(T);
		};

		template<class T> LocalData<T>::LocalData()
			: SyncData<T>() {}
		template<class T> LocalData<T>::LocalData(T data)
			: SyncData<T>(data) {}

		template<class T> class GhostData : public LocalData<T> {
			public:
				GhostData();
				GhostData(T);
				void update(T data);

		};

		template<class T> GhostData<T>::GhostData()
			: LocalData<T>() {}
		template<class T> GhostData<T>::GhostData(T data)
			: LocalData<T>(data) {}

		template<class T> void GhostData<T>::update(T data) {
			this->data = data;
		}
	}
}

namespace nlohmann {
	using FPMAS::graph::SyncData;

    template <class T>
    struct adl_serializer<SyncData<T>> {
		static void to_json(json& j, const SyncData<T>& data) {
			j = data.get();
		}

	};

	using FPMAS::graph::LocalData;

    template <class T>
    struct adl_serializer<LocalData<T>> {
		static LocalData<T> from_json(const json& j) {
			return LocalData<T>(j.get<T>());
		}

		static void to_json(json& j, const LocalData<T>& data) {
			j = data.get();
		}

	};
}
#endif
