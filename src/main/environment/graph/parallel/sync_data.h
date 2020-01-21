#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <nlohmann/json.hpp>
#include "zoltan_cpp.h"
#include "zoltan/zoltan_utils.h"

using nlohmann::json;

namespace FPMAS {
	namespace graph {
		namespace zoltan {
			namespace node {
				template<class T> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
				);
				template<class T, template<typename> class S> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
				);

			}
			namespace arc {
				template<class T> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
				);
				template<class T, template<typename> class S> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
				);
				template <class T, template<typename> class S> void mid_migrate_pp_fn(
					ZOLTAN_MID_POST_MIGRATE_ARGS
				);
			}
		}
		namespace synchro {

			template<class T> class None {
				public:
					/**
					 * In this mode, no overlapping zone is used.
					 *
					 * When a node is exported, its connections with nodes that are not
					 * exported on the same process are lost.
					 */
					const static zoltan::utils::zoltan_query_functions config;
			};
			template<class T> const zoltan::utils::zoltan_query_functions None<T>::config
				 (
						&FPMAS::graph::zoltan::node::post_migrate_pp_fn_no_sync<T>,
						&FPMAS::graph::zoltan::arc::post_migrate_pp_fn_no_sync<T>,
						NULL
						);


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

				/**
				 * In this mode, overlapping zones (represented as a "ghost graph")
				 * are built and synchronized on each proc.
				 *
				 * When a node is exported, ghost arcs and nodes are built to keep
				 * consistency across processes, and ghost nodes data is fetched
				 * at each GhostGraph::synchronize() call.
				 */
				const static zoltan::utils::zoltan_query_functions config;

		};
		template<class T> const zoltan::utils::zoltan_query_functions GhostData<T>::config
			 (
					&FPMAS::graph::zoltan::node::post_migrate_pp_fn_olz<T, GhostData>,
					&FPMAS::graph::zoltan::arc::post_migrate_pp_fn_olz<T, GhostData>,
					&FPMAS::graph::zoltan::arc::mid_migrate_pp_fn<T, GhostData>
					);

		template<class T> GhostData<T>::GhostData()
			: LocalData<T>() {}
		template<class T> GhostData<T>::GhostData(T data)
			: LocalData<T>(data) {}

		template<class T> void GhostData<T>::update(T data) {
			this->data = data;
		}
		}
	}
}

namespace nlohmann {
	using FPMAS::graph::synchro::SyncData;

    template <class T>
    struct adl_serializer<SyncData<T>> {
		static void to_json(json& j, const SyncData<T>& data) {
			j = data.get();
		}

	};

	using FPMAS::graph::synchro::LocalData;

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
