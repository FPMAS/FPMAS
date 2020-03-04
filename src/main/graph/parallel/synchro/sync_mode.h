#ifndef SYNC_MODE_H
#define SYNC_MODE_H

#include <memory>
#include <nlohmann/json.hpp>
#include "utils/macros.h"
#include "zoltan_cpp.h"
#include "../zoltan/zoltan_utils.h"
#include "communication/sync_communication.h"
#include "../proxy/proxy.h"
#include "graph/base/node.h"

using FPMAS::graph::parallel::proxy::Proxy;
using FPMAS::communication::SyncMpiCommunicator;

namespace FPMAS::graph::parallel {
	namespace zoltan {
		namespace node {
			template<typename T, int N> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template<typename T, int N, SYNC_MODE> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);

		}
		namespace arc {
			template<typename T, int N> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template<typename T, int N, SYNC_MODE> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template<typename T, int N, SYNC_MODE> void mid_migrate_pp_fn(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
		}
	}

	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	namespace synchro {
		using parallel::zoltan::utils::zoltan_query_functions;

		template<
			template<typename, int> class Mode,
			template<typename> class Wrapper,
			typename T,
			int N
		> class SyncMode {
			private:
				zoltan_query_functions _config;
			public:

				SyncMode(zoltan_query_functions config) : _config(config) {}

				const zoltan_query_functions& config() const;

				virtual void termination(DistributedGraph<T, Mode, N>& dg) {};

				static Wrapper<T>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&);
				static Wrapper<T>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&, const T&);
				static Wrapper<T>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&, T&&);
			};
		template<
			template<typename, int> class Mode,
			template<typename> class Wrapper,
			typename T,
			int N
				> const zoltan_query_functions& SyncMode<Mode, Wrapper, T, N>::config() const {
					return _config;
				}
		template<
			template<typename, int> class Mode,
			template<typename> class Wrapper,
			typename T,
			int N
				> Wrapper<T>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy
						) {
					return new Wrapper<T>(id, comm, proxy);
				}

		template<
			template<typename, int> class Mode,
			template<typename> class Wrapper,
			typename T,
			int N
				> Wrapper<T>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy, const T& data
						) {
					return new Wrapper<T>(id, comm, proxy, data);
				}

		template<
			template<typename, int> class Mode,
			template<typename> class Wrapper,
			typename T,
			int N
				> Wrapper<T>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy, T&& data
						) {
					return new Wrapper<T>(id, comm, proxy, std::move(data));
				}

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
			friend nlohmann::adl_serializer<SyncData<T>>;
			protected:
			/**
			 * Local representation of wrapped data.
			 */
			T data;
			SyncData();
			SyncData(const T&);
			SyncData(T&&);

			public:
			const T& get() const;
			T&& move();
			void update(T&& data);

			/**
			 * Returns a reference to the wrapped data. (default behavior)
			 *
			 * Should be overriden by extending classes to perform an acquire operation
			 * on the wrapped data.
			 *
			 * @return reference to wrapped data
			 */
			virtual T& acquire();

			/**
			 * Returns a const reference to the wrapped data. (default behavior)
			 *
			 * Should be overriden by extending classes to perform a read operation
			 * on the wrapped data.
			 *
			 * @return const reference to wrapped data
			 */
			virtual const T& read() ;

			virtual void release();
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
		template<class T> SyncData<T>::SyncData(T&& data) : data(std::move(data)) {
		}
		template<class T> SyncData<T>::SyncData(const T& data) : data(data) {
		}

		/**
		 * A direct const access reference to the wrapped data as it is currently
		 * physically represented is this SyncData instance.
		 *
		 * @return const reference to the current data
		 */
		template<class T> const T& SyncData<T>::get() const {
			return this->data;
		}

		template<class T> T&& SyncData<T>::move() {
			return std::move(this->data);
		}

		/**
		 * Updates internal data with the provided value.
		 *
		 * This function *should not* be used directly by a user, the proper
		 * way to edit data being acquire() -> perform some modifications ->
		 * release().
		 *
		 * This function is only used internally to actually updating data once
		 * it has been released.
		 *
		 * @param data updated data
		 */
		template<class T> void SyncData<T>::update(T&& data) {
			this->data = std::move(data);
		}

		/**
		 * Default getter, returns a reference to the wrapped data.
		 *
		 * @return reference to wrapped data
		 */
		template<class T> T& SyncData<T>::acquire() {
			return data;
		}

		/**
		 * Default const getter, returns a const reference to the wrapped data.
		 *
		 * @return const reference to wrapped data
		 */
		template<class T> const T& SyncData<T>::read() {
			return data;
		}

		/**
		 * Releases data, allowing other procs to acquire and read it.
		 */
		template<class T> void SyncData<T>::release() {}

	}
}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::SyncData;
	/**
	 * Any SyncData instance (and so instances of extending classes) can be
	 * json serialized to perform node migrations or data request responses.
	 *
	 * However, it is not possible to directly unserialized a SyncData
	 * instance, because this data wrapper is not supposed to be used directly,
	 * but must be unserialized as a concrete LocalData instance for example.
	 */
	template <class T>
		struct adl_serializer<std::unique_ptr<SyncData<T>>> {
			/**
			 * Serializes the data wrapped in the provided SyncData instance.
			 *
			 * This function is automatically called by the nlohmann library.
			 *
			 * @param j json to serialize to
			 * @param data reference to SyncData to serialize
			 */
			static void to_json(json& j, const std::unique_ptr<SyncData<T>>& data) {
				j = data->get();
			}

			static SyncData<T> from_json(const json& j) {
				return SyncData<T>(j.get<T>());
			}
		};
}


#endif
