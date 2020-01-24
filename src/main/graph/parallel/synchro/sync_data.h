#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <memory>
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


			public:

				virtual const T& get() const;

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

		template<class T> const T& SyncData<T>::get() const {
			return this->data;
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
		 * An std::shared_ptr<SyncData<T>*> might have been used, but would not
		 * have been optimal.
		 */
		template<class T> class SyncDataPtr {
			private:
				std::shared_ptr<SyncData<T>> syncData;
				//int refs = 0;
				// SyncDataPtr(const SyncDataPtr&);

			public:
				SyncDataPtr();
				SyncDataPtr(SyncData<T>*);
				SyncData<T>* const operator->();
				const SyncData<T>* const operator->() const;
				~SyncDataPtr();

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

		/**
		 * Copy constructor.
		 *
		 * Gives the ownership of the managed pointer to this instance : the
		 * managed pointer will be deleted when this instance will be.
		 *
		 * @param from Other instance to take the pointer from
		 */
		/*
		 *template<class T> SyncDataPtr<T>::SyncDataPtr(const SyncDataPtr<T>& from) {
		 *    this->syncData = from.syncData;
		 *    // this->refs = from.refs + 1;
		 *}
		 */

		/**
		 * Member of pointer operator, that allows easy access to the
		 * SyncData<T> pointer contained in this SyncDataPtr.
		 *
		 * The return pointer is `const` so that the internal pointer can't be
		 * changed externally.
		 *
		 * @return const pointer to the managed SyncData<T> instance
		 */
		template<class T> SyncData<T>* const SyncDataPtr<T>::operator->() {
			return this->syncData.get();
		}

		/**
		 * Const version of the member of pointer operator.
		 *
		 * @return const pointer to the const SyncData<T> instance
		 */
		template<class T> const SyncData<T>* const SyncDataPtr<T>::operator->() const {
			return this->syncData.get();
		}

		/**
		 * SyncDataPtr destructor.
		 *
		 * Deletes the managed pointer if this instance has ownership over it.
		 */
		template<class T> SyncDataPtr<T>::~SyncDataPtr() {
			/*
			 *this->refs -= 1;
			 *if(refs == 0)
			 *    delete syncData;
			 */
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
