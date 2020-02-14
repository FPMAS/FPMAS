#ifndef SYNC_DATA_H
#define SYNC_DATA_H

#include <memory>
#include <nlohmann/json.hpp>
#include "utils/macros.h"
#include "zoltan_cpp.h"
#include "../zoltan/zoltan_utils.h"
#include "communication/sync_communication.h"

using FPMAS::communication::SyncMpiCommunicator;

namespace FPMAS::graph::parallel {
	namespace zoltan {
		namespace node {
			template<NODE_PARAMS> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template<NODE_PARAMS, SYNC_MODE> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);

		}
		namespace arc {
			template<NODE_PARAMS> void post_migrate_pp_fn_no_sync(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template<NODE_PARAMS, SYNC_MODE> void post_migrate_pp_fn_olz(
					ZOLTAN_MID_POST_MIGRATE_ARGS
					);
			template <NODE_PARAMS, SYNC_MODE> void mid_migrate_pp_fn(
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
		template <NODE_PARAMS> class SyncData {

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
		template<NODE_PARAMS> SyncData<NODE_PARAMS_SPEC>::SyncData() {
		}

		/**
		 * Builds a SyncData instance initialized with the specified data.
		 *
		 * @param data data to wrap.
		 */
		template<NODE_PARAMS> SyncData<NODE_PARAMS_SPEC>::SyncData(T data) : data(data) {
		}

		/**
		 * A direct const access reference to the wrapped data as it is currently
		 * physically represented is this SyncData instance.
		 *
		 * @return const reference to the current data
		 */
		template<NODE_PARAMS> const T& SyncData<NODE_PARAMS_SPEC>::get() const {
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
		template<NODE_PARAMS> void SyncData<NODE_PARAMS_SPEC>::update(T data) {
			this->data = data;
		}

		/**
		 * Default getter, returns a reference to the wrapped data.
		 *
		 * @return reference to wrapped data
		 */
		template<NODE_PARAMS> T& SyncData<NODE_PARAMS_SPEC>::acquire() {
			return data;
		}

		/**
		 * Default const getter, returns a const reference to the wrapped data.
		 *
		 * @return const reference to wrapped data
		 */
		template<NODE_PARAMS> const T& SyncData<NODE_PARAMS_SPEC>::read() {
			return data;
		}

		/**
		 * Releases data, allowing other procs to acquire and read it.
		 */
		template<NODE_PARAMS> void SyncData<NODE_PARAMS_SPEC>::release() {}

		/**
		 * Smart pointer used as node data to store SyncData<NODE_PARAMS_SPEC> instances in
		 * the DistributedGraph.
		 */
		/*
		 * Using SyncData<NODE_PARAMS_SPEC>* pointers directly has Node data in the
		 * DistributedGraph (and in the GhostGraph) would require extra memory
		 * management and storage at the Graph scale to manually delete (and copy, and
		 * assign, etc) node data. Using this smart pointer instead avoid this,
		 * because its destructor is automatically and properly called as
		 * defined at the Graph scale as nodes are created or deleted.
		 * TODO: Might be optimized thanks to move semantics?
		 */
		template<NODE_PARAMS> class SyncDataPtr {
			private:
				std::shared_ptr<SyncData<NODE_PARAMS_SPEC>> syncData;

			public:
				SyncDataPtr();
				SyncDataPtr(SyncData<NODE_PARAMS_SPEC>*);

				/**
				 * Member of pointer operator, that allows easy access to the
				 * SyncData<NODE_PARAMS_SPEC> pointer contained in this SyncDataPtr.
				 *
				 * The return pointer is `const` so that the internal pointer can't be
				 * changed externally.
				 *
				 * @return const pointer to the managed SyncData<NODE_PARAMS_SPEC> instance
				 */
				SyncData<NODE_PARAMS_SPEC>* const operator->();

				/**
				 * Const version of the member of pointer operator.
				 *
				 * @return const pointer to the const SyncData<NODE_PARAMS_SPEC> instance
				 */
				const SyncData<NODE_PARAMS_SPEC>* const operator->() const;

		};


		/**
		 * Default constructor.
		 *
		 * Builds a void SyncDataPtr instance.
		 */
		template<NODE_PARAMS> SyncDataPtr<NODE_PARAMS_SPEC>::SyncDataPtr(){
		}

		/**
		 * Builds a SyncDataPtr instance that manages the provided SyncData
		 * pointer.
		 *
		 * @param syncData SyncData<NODE_PARAMS_SPEC> pointer
		 */
		template<NODE_PARAMS> SyncDataPtr<NODE_PARAMS_SPEC>::SyncDataPtr(SyncData<NODE_PARAMS_SPEC>* syncData)
			: syncData(std::shared_ptr<SyncData<NODE_PARAMS_SPEC>>(syncData)) {}

		template<NODE_PARAMS> SyncData<NODE_PARAMS_SPEC>* const SyncDataPtr<NODE_PARAMS_SPEC>::operator->() {
			return this->syncData.get();
		}

		template<NODE_PARAMS> const SyncData<NODE_PARAMS_SPEC>* const SyncDataPtr<NODE_PARAMS_SPEC>::operator->() const {
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
    template <NODE_PARAMS>
    struct adl_serializer<SyncDataPtr<NODE_PARAMS_SPEC>> {
		/**
		 * Serializes the data wrapped in the provided SyncData instance.
		 *
		 * This function is automatically called by the nlohmann library.
		 *
		 * @param j json to serialize to
		 * @param data reference to SyncData to serialize
		 */
		static void to_json(json& j, const SyncDataPtr<NODE_PARAMS_SPEC>& data) {
			j = data->get();
		}

	};
}
#endif
