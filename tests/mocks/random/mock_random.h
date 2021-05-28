#ifndef FPMAS_MOCK_RANDOM_H
#define FPMAS_MOCK_RANDOM_H

#include "gmock/gmock.h"
#include "fpmas/api/random/distribution.h"

class MockGenerator : public fpmas::api::random::Generator {
	public:
		MOCK_METHOD(result_type, min, (), (override));
		MOCK_METHOD(result_type, max, (), (override));
		MOCK_METHOD(result_type, call, (), ());

		result_type operator()() override {
			return call();
		}
};

template<typename T>
class MockDistribution : public fpmas::api::random::Distribution<T> {
	public:
		MOCK_METHOD(T, call, (fpmas::api::random::Generator&), ());

		T operator()(fpmas::api::random::Generator& gen) {
			return call(gen);
		}

		MOCK_METHOD(T, min, (), (const));
		MOCK_METHOD(T, max, (), (const));
};

#endif
