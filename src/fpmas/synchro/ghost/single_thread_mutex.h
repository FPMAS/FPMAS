#ifndef FPMAS_SINGLE_THREAD_MUTEX_H
#define FPMAS_SINGLE_THREAD_MUTEX_H

#include "fpmas/api/synchro/sync_mode.h"

/** \file src/fpmas/synchro/ghost/single_thread_mutex.h
 * Defines a Mutex class with no concurrency requirement.
 */

namespace fpmas { namespace synchro { namespace ghost {
	
	/**
	 * A base Mutex implementation designed to work in a single threaded
	 * environment. In consequence, no concurrency management is required.
	 */
	template<typename T>
		class SingleThreadMutex : public api::synchro::Mutex<T> {
			private:
				T& _data;
				void _lock() override {};
				void _lockShared() override {};
				void _unlock() override {};
				void _unlockShared() override {};

			public:
				/**
				 * SingleThreadMutex constructor.
				 *
				 * @param data reference to node data
				 */
				SingleThreadMutex(T& data) : _data(data) {}

				T& data() override {return _data;}
				const T& data() const override {return _data;}

				void lock() override {};
				void unlock() override {};
				bool locked() const override {return false;}

				void lockShared() override {};
				void unlockShared() override {};
				int sharedLockCount() const override {return 0;};
		};

}}}
#endif
