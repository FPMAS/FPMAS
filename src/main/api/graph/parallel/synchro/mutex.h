#ifndef MUTEX_H
#define MUTEX_H

namespace FPMAS::api::graph::parallel::synchro {
	template<typename T>
	class Mutex {
		public:
			virtual const T& read() = 0;
			virtual T& acquire() = 0;
			virtual void release(T&&) = 0;

			virtual void lock() = 0;
			virtual void unlock() = 0;
	};
}

#endif
