#ifndef FPMAS_MUTEX_API_H
#define FPMAS_MUTEX_API_H

/** \file src/fpmas/api/synchro/mutex.h
 * Mutex API
 */

#include "fpmas/api/graph/distributed_id.h"

namespace fpmas { namespace api { namespace synchro {
	/**
	 * Mutex API.
	 *
	 * A Mutex protects (and allows) access to data instance of type T, using
	 * four semantic access rules :
	 *   - read
	 *   - acquire
	 *   - lock
	 *   - lockShared
	 *
	 * In any case, concurrent access and data update management (potentially
	 * involving communications with other processes) are implementation
	 * defined, to allow the definition of several synchronization modes.
	 */
	template<typename T>
	class Mutex {
		protected:
			/**
			 * Internal lock function.
			 */
			virtual void _lock() = 0;
			/**
			 * Internal lock shared function.
			 */
			virtual void _lockShared() = 0;
			/**
			 * Internal unlock function.
			 */
			virtual void _unlock() = 0;
			/**
			 * Internal unlock shared function.
			 */
			virtual void _unlockShared() = 0;

		public:
			/**
			 * Direct access to internal data representation.
			 *
			 * **Does not guarantee any concurrent access or data update
			 * management.**
			 *
			 * @return direct reference to internal data
			 */
			virtual T& data() = 0;

			/**
			 * \copydoc data()
			 */
			virtual const T& data() const = 0;

			/**
			 * Provides a read access to the internal data.
			 *
			 * The functions blocks until data is effectively read.
			 *
			 * Read data **must** be released using releaseRead().
			 *
			 * @return reference to read data
			 */
			virtual const T& read() = 0;

			/**
			 * Releases a mutex previously accessed with read().
			 */
			virtual void releaseRead() = 0;

			/**
			 * Acquires an exclusive access to the internal data.
			 *
			 * The function blocks until data is effectively acquired.
			 *
			 * Write operations are semantically allowed on acquired data.
			 * However, how write operations are handled is implementation
			 * defined.
			 *
			 * Acquired data **must** be released using releaseAcquire().
			 *
			 * @return reference to acquired data
			 */
			virtual T& acquire() = 0;

			/**
			 * Releases a mutex previously accessed with acquire().
			 *
			 * Semantically, this operation also "commits" eventual write
			 * operations that were performed on the acquired data. However,
			 * the actual behavior is implementation defined.
			 */
			virtual void releaseAcquire() = 0;

			/**
			 * Locks the mutex.
			 *
			 * This function blocks until internal data is effectively locked. Upon
			 * return, an exclusive access to the internal data is guaranteed.
			 * However, contrary to the acquire() function, this operation does
			 * not involve any data update or write operation handling.
			 *
			 * A locked mutex **must** be unlocked using unlock().
			 */
			virtual void lock() = 0;
			/**
			 * Unlocks a mutex previously locked with lock().
			 */
			virtual void unlock() = 0;
			/**
			 * Returns true if and only if the mutex has been locked and not
			 * yet unlocked.
			 *
			 * @return true iff mutex is locked
			 */
			virtual bool locked() const = 0;

			/**
			 * Obtains a shared lock access to the mutex.
			 *
			 * This function blocks until a shared lock access to mutex is
			 * effectively obtained. Contrary to lock(), multiple threads are
			 * allowed to simultaneously obtain a shared lock to the mutex.
			 *
			 * The mutex **must** then be unlocked using unlockShared().
			 */
			virtual void lockShared() = 0;

			/**
			 * Unlocks a mutex previously locked with lockShared().
			 *
			 * Semantically, all threads that called lockShared() must call
			 * unlockShared() to finally unlock the mutex.
			 */
			virtual void unlockShared() = 0;

			/**
			 * Returns the count of threads that currently own a shared lock
			 * access to the mutex.
			 *
			 * @return shared lock count
			 */
			virtual int sharedLockCount() const = 0;

			virtual ~Mutex() {}
	};
}}}
#endif
