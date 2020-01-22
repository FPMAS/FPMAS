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
		/**
		 * The synchro namespace defines data structures used to manage
		 * data synchronization accross processes.
		 */
		namespace synchro {

			/**
			 * Synchronisation mode that can be used as a template argument for
			 * a DistributedGraph, where no synchronization or overlapping zone
			 * is used.
			 *
			 * When a node is exported, its connections with nodes that are not
			 * exported on the same process are lost.
			 */
			template<class T> class None {
				public:
					/**
					 * Zoltan config associated to the None synchronization
					 * mode.
					 */
					const static zoltan::utils::zoltan_query_functions config;
			};
			template<class T> const zoltan::utils::zoltan_query_functions None<T>::config
				 (
						&FPMAS::graph::zoltan::node::post_migrate_pp_fn_no_sync<T>,
						&FPMAS::graph::zoltan::arc::post_migrate_pp_fn_no_sync<T>,
						NULL
						);


		/**
		 * Abstract wrapper for data contained in a DistributedGraph.
		 *
		 * Getters are used to access data in a completely generic way.
		 *
		 * Extending classes, that define how synchronization is really
		 * handled, implement different behaviors for those getters, depending
		 * on the fact that the data in truly local or located on an other
		 * process. However, those mechanics remains fully transparent for the
		 * user at the graph / node scale.
		 *
		 * @tparam T wrapped data type
		 */
		template <class T> class SyncData {
			protected:
				/**
				 * Local representation of wrapped data.
				 */
				T data;
			public:
				SyncData();
				SyncData(T);

				/**
				 * Returns a reference to the wrapped data. (default behavior)
				 *
				 * Should be overriden by extending classes to perform an acquire operation
				 * on the wrapped data.
				 *
				 * @return reference to wrapped data
				 */
				virtual T& get();

				/**
				 * Returns a const reference to the wrapped data. (default behavior)
				 *
				 * Should be overriden by extending classes to perform a read operation
				 * on the wrapped data.
				 *
				 * @return const reference to wrapped data
				 */
				virtual const T& get() const;
		};

		/**
		 * Default constructor.
		 */
		template<class T> SyncData<T>::SyncData() {
		}

		/**
		 * Builds a SyncData instance initialized with the specified data.
		 *
		 * @param data data to wrap.
		 */
		template<class T> SyncData<T>::SyncData(T data) : data(data) {
		}


		template<class T> T& SyncData<T>::get() {
			return data;
		}


		template<class T> const T& SyncData<T>::get() const {
			return data;
		}


		// TODO : all the constructors might be defined private, because the
		// user is never supposed to manually instanciate wrappers.
		/**
		 * Wrapper used for local nodes of a DistributedGraph.
		 *
		 * Wrapped data is truely local, and getters used are the defaults
		 * defined in SyncData.
		 */
		template<class T> class LocalData : public SyncData<T> {
			public:
				LocalData();
				LocalData(T);
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
		template<class T> LocalData<T>::LocalData(T data)
			: SyncData<T>(data) {}

		/**
		 * Synchronisation mode used as default by the DistributedGraph, and
		 * wrapper for distant data represented as "ghosts".
		 *
		 * In this mode, overlapping zones (represented as a "ghost graph")
		 * are built and synchronized on each proc.
		 *
		 * When a node is exported, ghost arcs and nodes are built to keep
		 * consistency across processes, and ghost nodes data is fetched
		 * at each GhostGraph::synchronize() call.
		 *
		 * Getters used are actually the defaults provided by SyncData and
		 * LocalData, because once data has been fetched from other procs, it
		 * can be accessed locally.
		 *
		 * In consequence, such "ghost" data can be read and written as any
		 * other LocalData. However, modifications **won't be reported** to the
		 * origin proc, and will be overriden at the next synchronization.
		 *
		 */
		template<class T> class GhostData : public LocalData<T> {
			public:
				GhostData();
				GhostData(T);

				/**
				 * Defines the Zoltan configuration used manage and migrate
				 * GhostNode s and GhostArc s.
				 */
				const static zoltan::utils::zoltan_query_functions config;

		};
		template<class T> const zoltan::utils::zoltan_query_functions GhostData<T>::config
			 (
					&FPMAS::graph::zoltan::node::post_migrate_pp_fn_olz<T, GhostData>,
					&FPMAS::graph::zoltan::arc::post_migrate_pp_fn_olz<T, GhostData>,
					&FPMAS::graph::zoltan::arc::mid_migrate_pp_fn<T, GhostData>
					);

		/**
		 * Default constructor.
		 */
		template<class T> GhostData<T>::GhostData()
			: LocalData<T>() {}

		/**
		 * Builds a GhostData instance initialized with the specified data.
		 *
		 * @param data data to wrap
		 */
		template<class T> GhostData<T>::GhostData(T data)
			: LocalData<T>(data) {}
		}
	}
}

namespace nlohmann {
	using FPMAS::graph::synchro::SyncData;

	/**
	 * Any SyncData instance (and so instances of extending classes) can be
	 * json serialized to perform node migrations or data request responses.
	 *
	 * However, it is not possible to directly unserialized a SyncData
	 * instance, because this data wrapper is not supposed to be used directly,
	 * but must be unserialized as a concrete LocalData instance for example.
	 */
    template <class T>
    struct adl_serializer<SyncData<T>> {
		/**
		 * Serializes the data wrapped in the provided SyncData instance.
		 *
		 * This function is automatically called by the nlohmann library.
		 *
		 * @param j json to serialize to
		 * @param data reference to SyncData to serialize
		 */
		static void to_json(json& j, const SyncData<T>& data) {
			j = data.get();
		}

	};

	using FPMAS::graph::synchro::LocalData;

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
