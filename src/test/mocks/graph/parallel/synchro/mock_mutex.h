#ifndef MOCK_MUTEX_H
#define MOCK_MUTEX_H

#include "gmock/gmock.h"

#include "api/graph/parallel/synchro/mutex.h"

template<typename T>
class MockMutex : public virtual FPMAS::api::graph::parallel::synchro::Mutex<T> {
	public:
		MockMutex() {}
		MockMutex(T&) {}
		MockMutex(const MockMutex<T>&) {}
		MockMutex<T>& operator=(const MockMutex<T>&) {return *this;}

		MOCK_METHOD(void, _lock, (), (override));
		MOCK_METHOD(void, _unlock, (), (override));

		MOCK_METHOD(T&, data, (), (override));

		MOCK_METHOD(const T&, read, (), (override));
		MOCK_METHOD(T&, acquire, (), (override));
		MOCK_METHOD(void, release, (), (override));

		MOCK_METHOD(void, lock, (), (override));
		MOCK_METHOD(void, unlock, (), (override));
		MOCK_METHOD(bool, locked, (), (const, override));
};

#endif
