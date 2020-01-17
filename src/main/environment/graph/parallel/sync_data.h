#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include "distributed_graph.h"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace FPMAS {
	namespace graph {
		template<class T> class SyncData;
	}
}

namespace nlohmann {
	using FPMAS::graph::SyncData;

    template <class T>
    struct adl_serializer<SyncData<T>> {
		static SyncData<T> from_json(const json& j) {
			return SyncData(from_json(j));
		}

		static void to_json(json& j, const SyncData<T>& data) {
			to_json(j, data.get());
		}

	};
}

namespace FPMAS {
	namespace graph {

		template <class T> class SyncData {
			public:
				SyncData(T* data);
				virtual T& get() = 0;

		};

/*
 *        template <class T> class SyncDistributedGraph : public DistributedGraph<SyncData<T>> {
 *            public:
 *                SyncDistributedGraph();
 *
 *                Node<T>* buildNode(unsigned long id, T* data);
 *                Node<T>* buildNode(unsigned long id, float weight, T* data);
 *
 *        };
 */

		template<class T> SyncData<T>::SyncData(T* data) {

		}

		template<class T> T& SyncData<T>::get() {
			return this->data;
		}

/*
 *        template<class T> SyncDistributedGraph<T>::SyncDistributedGraph()
 *            : DistributedGraph<SyncData<T>>() {
 *            }
 *
 *        template<class T> Node<T>* SyncDistributedGraph<T>::buildNode(unsigned long id, T* data) {
 *            return DistributedGraph<SyncData<T>>::buildNode(id, new SyncData(data));
 *        }
 */
	}
}
#endif
