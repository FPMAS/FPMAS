#ifndef MUTEX_API_H
#define MUTEX_API_H

#include "fpmas/api/graph/distributed_id.h"

namespace fpmas { namespace api { namespace synchro {
	template<typename T>
	class Mutex {
		protected:
			virtual void _lock() = 0;
			virtual void _lockShared() = 0;
			virtual void _unlock() = 0;
			virtual void _unlockShared() = 0;

		public:
			virtual T& data() = 0;
			virtual const T& data() const = 0;

			virtual const T& read() = 0;
			virtual void releaseRead() = 0;

			virtual T& acquire() = 0;
			virtual void releaseAcquire() = 0;

			virtual void lock() = 0;
			virtual void unlock() = 0;
			virtual bool locked() const = 0;

			virtual void lockShared() = 0;
			virtual void unlockShared() = 0;
			virtual int lockedShared() const = 0;

			virtual ~Mutex() {}
	};
}}}
#endif
