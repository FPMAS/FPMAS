#ifndef FPMAS_RANDOM_GENERATOR_H
#define FPMAS_RANDOM_GENERATOR_H

#include <cstdint>
#include <random>
#include "fpmas/api/random/generator.h"

namespace fpmas { namespace random {

	template<typename Generator_t>
	class Generator : public api::random::Generator {
		private:
			Generator_t gen;
		public:

			result_type min() override {
				return gen.min();
			}

			result_type max() override {
				return gen.max();
			}

			result_type operator()() override {
				return gen();
			}
	};

	typedef Generator<std::mt19937> mt19937;
	typedef Generator<std::mt19937_64> mt19937_64;
	typedef Generator<std::minstd_rand> minstd_rand;
	typedef Generator<std::random_device> random_device;
}}
#endif
