#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <memory>
#include <nlohmann/json.hpp>
#include "zoltan_cpp.h"
#include "../zoltan/zoltan_utils.h"
#include "communication/sync_communication.h"

using FPMAS::communication::SyncMpiCommunicator;

namespace FPMAS::graph::parallel {
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
				SyncData();
				SyncData(T);

			public:
				const T& get() const;
				void update(T data);

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
		template<class T> SyncData<T>::SyncData(T data) : data(data) {
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
		template<class T> void SyncData<T>::update(T data) {
			this->data = data;
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

		/**
		 * Smart pointer used as node data to store SyncData<T> instances in
		 * the DistributedGraph.
		 */
		/*
		 * Using SyncData<T>* pointers directly has Node data in the
		 * DistributedGraph (and in the GhostGraph) would require extra memory
		 * management and storage at the Graph scale to manually delete (and copy, and
		 * assign, etc) node data. Using this smart pointer instead avoid this,
		 * because its destructor is automatically and properly called as
		 * defined at the Graph scale as nodes are created or deleted.
		 * TODO: Might be optimized thanks to move semantics?
		 */
		template<class T> class SyncDataPtr {
			private:
				std::shared_ptr<SyncData<T>> syncData;

			public:
				SyncDataPtr();
				SyncDataPtr(SyncData<T>*);

				/**
				 * Member of pointer operator, that allows easy access to the
				 * SyncData<T> pointer contained in this SyncDataPtr.
				 *
				 * The return pointer is `const` so that the internal pointer can't be
				 * changed externally.
				 *
				 * @return const pointer to the managed SyncData<T> instance
				 */
				SyncData<T>* const operator->();

				/**
				 * Const version of the member of pointer operator.
				 *
				 * @return const pointer to the const SyncData<T> instance
				 */
				const SyncData<T>* const operator->() const;

		};


		/**
		 * Default constructor.
		 *
		 * Builds a void SyncDataPtr instance.
		 */
		template<class T> SyncDataPtr<T>::SyncDataPtr(){
		}

		/**
		 * Builds a SyncDataPtr instance that manages the provided SyncData
		 * pointer.
		 *
		 * @param syncData SyncData<T> pointer
		 */
		template<class T> SyncDataPtr<T>::SyncDataPtr(SyncData<T>* syncData)
			: syncData(std::shared_ptr<SyncData<T>>(syncData)) {}

		template<class T> SyncData<T>* const SyncDataPtr<T>::operator->() {
			return this->syncData.get();
		}

		template<class T> const SyncData<T>* const SyncDataPtr<T>::operator->() const {
			return this->syncData.get();
		}
	}
}

namespace nlohmann {
	using FPMAS::graph::parallel::synchro::SyncDataPtr;

	/**
	 * Any SyncData instance (and so instances of extending classes) can be
	 * json serialized to perform node migrations or data request responses.
	 *
	 * However, it is not possible to directly unserialized a SyncData
	 * instance, because this data wrapper is not supposed to be used directly,
	 * but must be unserialized as a concrete LocalData instance for example.
	 */
    template <class T>
    struct adl_serializer<SyncDataPtr<T>> {
		/**
		 * Serializes the data wrapped in the provided SyncData instance.
		 *
		 * This function is automatically called by the nlohmann library.
		 *
		 * @param j json to serialize to
		 * @param data reference to SyncData to serialize
		 */
		static void to_json(json& j, const SyncDataPtr<T>& data) {
			j = data->get();
		}

	};
}
#endif
