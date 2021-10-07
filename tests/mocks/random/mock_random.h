#ifndef FPMAS_MOCK_RANDOM_H
#define FPMAS_MOCK_RANDOM_H

#include "gmock/gmock.h"
#include "fpmas/api/random/distribution.h"

class MockGenerator : public fpmas::api::random::Generator<std::uint_fast64_t> {
	public:
		typedef std::uint_fast64_t result_type;

		MOCK_METHOD(result_type, call, (), ());

		static constexpr result_type min() {
			return 0;
		}

		static constexpr result_type max() {
			return std::numeric_limits<result_type>::max();
		}

		result_type operator()() override {
			return call();
		}
};

template<typename T>
class MockDistribution : public fpmas::api::random::Distribution<T> {
	public:
		MOCK_METHOD(T, call, (), ());

		T operator()(fpmas::api::random::Generator<std::uint64_t>& gen) {
			return call();
		}
		T operator()(fpmas::api::random::Generator<std::uint32_t>& gen) {
			return call();
		}

		MOCK_METHOD(T, min, (), (const));
		MOCK_METHOD(T, max, (), (const));
};

#endif
