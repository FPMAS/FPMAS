#ifndef FPMAS_SYNCHRO_GUARDS
#define FPMAS_SYNCHRO_GUARDS

#include "fpmas/api/graph/distributed_node.h"

namespace fpmas { namespace synchro {
	/**
	 * Generic Guard API.
	 *
	 * Guards provides an elegant and reliable way to perform operations on
	 * each node mutex.
	 *
	 * The concept of the guard is to acquire lock and/or resources when the
	 * Guard is instantiated, and to automatically release it when the guard
	 * goes out of scope.
	 *
	 * This concept is directly inspired from the [std::lock_guard
	 * class](https://en.cppreference.com/w/cpp/thread/lock_guard).
	 */
	template<typename T>
		class Guard {
			protected:
				api::synchro::Mutex<T>& mutex;

				/**
				 * Guard constructor.
				 *
				 * @param node node on which operation will be performed
				 */
				Guard(api::graph::DistributedNode<T>* node)
					: mutex(*node->mutex()) {
					}

			public:
				Guard(const Guard&) = delete;
				Guard(Guard&&) = delete;
				Guard& operator=(const Guard&) = delete;
				Guard& operator=(Guard&&) = delete;

				virtual ~Guard() {}
		};

	/**
	 * ReadGuard.
	 */
	template<typename T>
		class ReadGuard : Guard<T> {
			public:
				/**
				 * ReadGuard constructor.
				 *
				 * The node is read exactly as if `node->mutex()->read()` was
				 * called. The internal node data is then updated according to
				 * the current SyncMode, and can be safely accessed with
				 * `node->data()` until this Guard goes out of scope.
				 *
				 * @param node to read
				 */
				ReadGuard(api::graph::DistributedNode<T>* node)
					: Guard<T>(node) {
						// updates node.data() according to the SyncMode
						this->mutex.read();
					}

				/**
				 * ReadGuard destructor.
				 *
				 * Releases the read operation.
				 */
				virtual ~ReadGuard() {
					this->mutex.releaseRead();
				}
		};

	/**
	 * AcquireGuard.
	 */
	template<typename T>
		class AcquireGuard : Guard<T> {
			public:
				/**
				 * AcquireGuard constructor.
				 *
				 * The node is acquired exactly as if `node->mutex()->acquire()` was
				 * called. The internal node data is then updated according to
				 * the current SyncMode, and can be safely accessed with
				 * `node->data()` until this Guard goes out of scope.
				 *
				 * @param node to acquire
				 */
				AcquireGuard(api::graph::DistributedNode<T>* node)
					: Guard<T>(node) {
						// updates node.data() according to the SyncMode
						this->mutex.acquire();
					}
				/**
				 * AcquireGuard destructor.
				 *
				 * Releases the acquire operation.
				 */
				virtual ~AcquireGuard() {
					this->mutex.releaseAcquire();
				}
		};

}}
#endif
