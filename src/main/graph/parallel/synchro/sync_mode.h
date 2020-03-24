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
	template<typename T, SYNC_MODE, int N> class DistributedGraphBase;

	/**
	 * Contains synchronization modes and data wrappers implementations.
	 */
	namespace synchro {
		using parallel::GhostNode;
		using parallel::zoltan::utils::zoltan_query_functions;

		/**
		 * Data wrappers used by the respective synchronization modes.
		 */
		namespace wrappers {

			/**
			 * Abstract wrapper for data contained in a DistributedGraphBase.
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
				friend std::string DistributedGraphBase<T,S,N>::getLocalData(DistributedId) const;
				friend std::string DistributedGraphBase<T,S,N>::getUpdatedData(DistributedId) const;
				friend GhostNode<T,N,S>::GhostNode(
						SyncMpiCommunicator&,
						Proxy&,
						Node<std::unique_ptr<SyncData<T,N,S>>, DistributedId, N>&
						);
				protected:
				/**
				 * Local wrapped data instance.
				 */
				T data;
				SyncData();
				SyncData(const T&);
				SyncData(T&&);
				/**
				 * A proteced direct access reference to the wrapped data as it is currently
				 * physically represented is this SyncData instance.
				 *
				 * @return reference to the current data
				 */
				T& get();
				/**
				 * A proteced direct access reference to the wrapped data as it is currently
				 * physically represented is this SyncData instance.
				 *
				 * @return reference to the current data
				 */
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
			 * Builds a SyncData instance initialized moving the specified data to this wrapper.
			 *
			 * @param data rvalue to data to wrap.
			 */
			template<typename T, int N, SYNC_MODE> SyncData<T,N,S>::SyncData(T&& data) : data(std::forward<T>(data)) {
			}

			/**
			 * Builds a SyncData instance initialized copying the specified data to this wrapper.
			 *
			 * @param data data to wrap.
			 */
			template<typename T, int N, SYNC_MODE> SyncData<T,N,S>::SyncData(const T& data) : data(data) {
			}

			
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
				this->data = std::forward<T>(data);
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
		}

		/**
		 * Contains class templates that might be used as synchronization mode.
		 */
		namespace modes {

			/**
			 * Abstact generic SyncMode class template.
			 *
			 * @tparam Mode implementing concrete class (GhostMode, NoSyncMode,
			 * HardSyncMode...)
			 * @tparam Wrapper associated data wrapper (wrappers::GhostData,
			 * wrappers::NoSyncData, wrappers::HardSyncData...)
			 * @tparam T wrapped data type
			 * @tparam N layers count
			 *
			 * @see wrappers
			 * @see wrappers::SyncData
			 */
			template<
				template<typename, int> class Mode,
				template<typename, int> class Wrapper,
				typename T,
				int N
					> class SyncMode {
						private:
							zoltan_query_functions _config;
						protected:
							/**
							 * Reference to the parent DistributedGraphBase
							 * instance.
							 */
							DistributedGraphBase<T, Mode, N>& dg;

							/**
							 * SyncMode constructor.
							 *
							 * @param config zoltan configuration associated to
							 * this mode
							 * @param dg parent DistributedGraphBase
							 */
							SyncMode(
									zoltan_query_functions config,
									DistributedGraphBase<T, Mode, N>& dg
									) : _config(config), dg(dg) {}

						public:
							const zoltan_query_functions& config() const;

							/**
							 * Synchronization mode dependent link request
							 * initialization.
							 *
							 * Called by the DistributedGraphBase before creating a
							 * new distant Arc, to acquire required resources
							 * for example.
							 *
							 * @param source source node id
							 * @param target target node id
							 * @param arcId new arc id
							 * @param layer layer id
							 */
							virtual void initLink(
									DistributedId source,
									DistributedId target,
									DistributedId arcId,
									LayerId layer) {}

							/**
							 * Synchronization mode dependent linked
							 * notification.
							 *
							 * Called after a new distant Arc has been created in the
							 * DistributedGraphBase, to export it to distant procs
							 * for example.
							 *
							 * @param arc pointer to the new arc
							 */
							virtual void notifyLinked(
								Arc<std::unique_ptr<wrappers::SyncData<T,N,Mode>>,DistributedId,N>* arc
								) {}

							/**
							 * fs
							 */
							virtual void initUnlink(
								Arc<std::unique_ptr<wrappers::SyncData<T,N,Mode>>,DistributedId,N>* arc
								) {}
							/**
							 * sdf
							 */
							virtual void notifyUnlinked(
								DistributedId source,
								DistributedId target,
								DistributedId arcId,
								LayerId layer) {}

							/**
							 * A synchronization mode dependent termination function,
							 * called at each DistributedGraphBase::synchronize() call.
							 */
							virtual void termination() {};

							static Wrapper<T,N>*
								wrap(DistributedId, SyncMpiCommunicator&, const Proxy&);
							static Wrapper<T,N>*
								wrap(DistributedId, SyncMpiCommunicator&, const Proxy&, const T&);
							static Wrapper<T,N>*
								wrap(DistributedId, SyncMpiCommunicator&, const Proxy&, T&&);
					};

			/**
			 * Returns the Zoltan query functions associated to this
			 * synchronization mode.
			 *
			 * @return Zoltan configuration
			 */
			template<
				template<typename, int> class Mode,
				template<typename, int> class Wrapper,
				typename T,
				int N
					> const zoltan_query_functions& SyncMode<Mode, Wrapper, T, N>::config() const {
						return _config;
					}

			/**
			 * Wraps the specified data in a new `Wrapper` instance.
			 */
			template<
				template<typename, int> class Mode,
				template<typename, int> class Wrapper,
				typename T,
				int N
					> Wrapper<T,N>* SyncMode<Mode, Wrapper, T, N>::wrap(
							DistributedId id, SyncMpiCommunicator& comm, const Proxy& proxy
							) {
						return new Wrapper<T,N>(id, comm, proxy);
					}
			/**
			 * Wraps the specified data in a new `Wrapper` instance.
			 */
			template<
				template<typename, int> class Mode,
				template<typename, int> class Wrapper,
				typename T,
				int N
					> Wrapper<T,N>* SyncMode<Mode, Wrapper, T, N>::wrap(
							DistributedId id, SyncMpiCommunicator& comm, const Proxy& proxy, const T& data
							) {
						return new Wrapper<T,N>(id, comm, proxy, data);
					}
			/**
			 * Wraps the specified data in a new `Wrapper` instance.
			 */
			template<
				template<typename, int> class Mode,
				template<typename, int> class Wrapper,
				typename T,
				int N
					> Wrapper<T,N>* SyncMode<Mode, Wrapper, T, N>::wrap(
							DistributedId id, SyncMpiCommunicator& comm, const Proxy& proxy, T&& data
							) {
						return new Wrapper<T,N>(id, comm, proxy, std::forward<T>(data));
					}

		}
	}
}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::wrappers::SyncData;
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
