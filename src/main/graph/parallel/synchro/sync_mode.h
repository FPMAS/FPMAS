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
#include "../olz.h"


using FPMAS::communication::SyncMpiCommunicator;

namespace FPMAS::graph::parallel {
	using base::Node;
	using base::Arc;
	using base::ArcId;
	using base::LayerId;
	using proxy::Proxy;

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
			template<typename T, int N, SYNC_MODE> void obj_size_multi_fn(
					ZOLTAN_OBJ_SIZE_ARGS
					);
			template<typename T, int N, SYNC_MODE> void pack_obj_multi_fn(
					ZOLTAN_PACK_OBJ_ARGS
					);
			template<typename T, int N, SYNC_MODE> void unpack_obj_multi_fn(
					ZOLTAN_UNPACK_OBJ_ARGS
					);
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

	template<typename T, int N, SYNC_MODE> class GhostNode;
	template<typename T, SYNC_MODE, int N> class DistributedGraph;

	namespace synchro {
		using parallel::GhostNode;
		using parallel::zoltan::utils::zoltan_query_functions;


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
		template <typename T, int N, SYNC_MODE> class SyncData {
			friend nlohmann::adl_serializer<std::unique_ptr<SyncData<T, N, S>>>;
			friend std::string DistributedGraph<T,S,N>::getLocalData(unsigned long) const;
			friend std::string DistributedGraph<T,S,N>::getUpdatedData(unsigned long) const;
			friend GhostNode<T,N,S>::GhostNode(
					SyncMpiCommunicator&,
					Proxy&,
					Node<std::unique_ptr<SyncData<T,N,S>>, N>&
					);
			protected:
			/**
			 * Local wrapped data instance.
			 */
			T data;
			SyncData();
			SyncData(const T&);
			SyncData(T&&);
			T& get();
			const T& get() const;

			public:
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
		template<typename T, int N, SYNC_MODE> SyncData<T,N,S>::SyncData() {
		}

		/**
		 * Builds a SyncData instance initialized with the specified data.
		 *
		 * @param data data to wrap.
		 */
		template<typename T, int N, SYNC_MODE> SyncData<T,N,S>::SyncData(T&& data) : data(std::move(data)) {
		}
		template<typename T, int N, SYNC_MODE> SyncData<T,N,S>::SyncData(const T& data) : data(data) {
		}

		/**
		 * A private direct access reference to the wrapped data as it is currently
		 * physically represented is this SyncData instance.
		 *
		 * @return reference to the current data
		 */
		template<typename T, int N, SYNC_MODE> const T& SyncData<T,N,S>::get() const {
			return this->data;
		}
		template<typename T, int N, SYNC_MODE> T& SyncData<T,N,S>::get() {
			return this->data;
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
		template<typename T, int N, SYNC_MODE> void SyncData<T,N,S>::update(T&& data) {
			this->data = std::move(data);
		}

		/**
		 * Default getter, returns a reference to the wrapped data.
		 *
		 * @return reference to wrapped data
		 */
		template<typename T, int N, SYNC_MODE> T& SyncData<T,N,S>::acquire() {
			return data;
		}

		/**
		 * Default const getter, returns a const reference to the wrapped data.
		 *
		 * @return const reference to wrapped data
		 */
		template<typename T, int N, SYNC_MODE> const T& SyncData<T,N,S>::read() {
			return data;
		}

		/**
		 * Releases data, allowing other procs to acquire and read it.
		 */
		template<typename T, int N, SYNC_MODE> void SyncData<T,N,S>::release() {}

		template<
			template<typename, int> class Mode,
			template<typename, int, SYNC_MODE> class Wrapper,
			typename T,
			int N
		> class SyncMode {
			private:
				zoltan_query_functions _config;
			protected:
				DistributedGraph<T, Mode, N>& dg;
			public:

				SyncMode(
					zoltan_query_functions config,
					DistributedGraph<T, Mode, N>& dg
					) : _config(config), dg(dg) {}

				const zoltan_query_functions& config() const;

				virtual void initLink(NodeId, NodeId, ArcId, LayerId) {}
				virtual void notifyLinked(Arc<std::unique_ptr<SyncData<T,N,Mode>>,N>*) {}

				/**
				 * A synchronization mode dependent termination function,
				 * called at each DistributedGraph::synchronize() call.
				 */
				virtual void termination() {};

				static Wrapper<T,N,Mode>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&);
				static Wrapper<T,N,Mode>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&, const T&);
				static Wrapper<T,N,Mode>*
					wrap(NodeId, SyncMpiCommunicator&, const Proxy&, T&&);
			};
		template<
			template<typename, int> class Mode,
			template<typename, int, SYNC_MODE> class Wrapper,
			typename T,
			int N
				> const zoltan_query_functions& SyncMode<Mode, Wrapper, T, N>::config() const {
					return _config;
				}
		template<
			template<typename, int> class Mode,
			template<typename, int, SYNC_MODE> class Wrapper,
			typename T,
			int N
				> Wrapper<T,N,Mode>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy
						) {
					return new Wrapper<T,N,Mode>(id, comm, proxy);
				}

		template<
			template<typename, int> class Mode,
			template<typename, int, SYNC_MODE> class Wrapper,
			typename T,
			int N
				> Wrapper<T,N,Mode>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy, const T& data
						) {
					return new Wrapper<T,N,Mode>(id, comm, proxy, data);
				}

		template<
			template<typename, int> class Mode,
			template<typename, int, SYNC_MODE> class Wrapper,
			typename T,
			int N
				> Wrapper<T,N,Mode>* SyncMode<Mode, Wrapper, T, N>::wrap(
						NodeId id, SyncMpiCommunicator& comm, const Proxy& proxy, T&& data
						) {
					return new Wrapper<T,N,Mode>(id, comm, proxy, std::move(data));
				}

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
	template <typename T, int N, SYNC_MODE>
		struct adl_serializer<std::unique_ptr<SyncData<T,N,S>>> {
			/**
			 * Serializes the data wrapped in the provided SyncData instance.
			 *
			 * This function is automatically called by the nlohmann library.
			 *
			 * @param j json to serialize to
			 * @param data reference to SyncData to serialize
			 */
			static void to_json(json& j, const std::unique_ptr<SyncData<T,N,S>>& data) {
				j = data->get();
			}

			static SyncData<T,N,S> from_json(const json& j) {
				return SyncData<T,N,S>(j.get<T>());
			}
		};
}


#endif
