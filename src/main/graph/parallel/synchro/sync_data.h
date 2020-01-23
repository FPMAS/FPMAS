#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <nlohmann/json.hpp>
#include "zoltan_cpp.h"
#include "../zoltan/zoltan_utils.h"
#include "communication/communication.h"

using FPMAS::communication::TerminableMpiCommunicator;

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
				SyncData(TerminableMpiCommunicator&);
				SyncData(TerminableMpiCommunicator&, T);

			public:

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

		template<class T> SyncData<T>::SyncData(TerminableMpiCommunicator&) {
		}

		template<class T> SyncData<T>::SyncData(T data) : data(data) {
		}

		/**
		 * Builds a SyncData instance initialized with the specified data.
		 *
		 * @param data data to wrap.
		 */
		template<class T> SyncData<T>::SyncData(TerminableMpiCommunicator&, T data) : data(data) {
		}


		template<class T> T& SyncData<T>::get() {
			return data;
		}


		template<class T> const T& SyncData<T>::get() const {
			return data;
		}

		template<class T> class SyncDataPtr {
			private:
				SyncData<T>* syncData;
				bool toDelete = true;

			public:
				SyncDataPtr();
				SyncDataPtr(SyncData<T>*);
				SyncDataPtr(SyncDataPtr&);
				SyncData<T>* const operator->();
				const SyncData<T>* const operator->() const;
				~SyncDataPtr();

		};

		template<class T> SyncDataPtr<T>::SyncDataPtr(){
			toDelete=false;
		}
		template<class T> SyncDataPtr<T>::SyncDataPtr(SyncData<T>* syncData)
			: syncData(syncData) {}
		template<class T> SyncDataPtr<T>::SyncDataPtr(SyncDataPtr<T>& from) {
			from.toDelete = false;
			this->syncData = from.syncData;
		}

		template<class T> SyncData<T>* const SyncDataPtr<T>::operator->() {
			return this->syncData;
		}
		template<class T> const SyncData<T>* const SyncDataPtr<T>::operator->() const {
			return this->syncData;
		}
		template<class T> SyncDataPtr<T>::~SyncDataPtr() {
			if(toDelete)
				delete syncData;
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
