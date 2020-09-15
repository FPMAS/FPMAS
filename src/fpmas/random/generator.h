#ifndef FPMAS_RANDOM_GENERATOR_H
#define FPMAS_RANDOM_GENERATOR_H

/** \file src/fpmas/random/generator.h
 * Random Generator implementation.
 */

#include <cstdint>
#include <random>
#include "fpmas/api/random/generator.h"

namespace fpmas { namespace random {

	/**
	 * api::random::Generator implementation.
	 */
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

	/**
	 * Predefined
	 * [mt19937](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine)
	 * engine.
	 */
	typedef Generator<std::mt19937> mt19937;
	/**
	 * Predefined
	 * [mt19937_64](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine)
	 * engine.
	 */
	typedef Generator<std::mt19937_64> mt19937_64;
	/**
	 * Predefined
	 * [minstd_rand](https://en.cppreference.com/w/cpp/numeric/random/linear_congruential_engine)
	 * engine.
	 */
	typedef Generator<std::minstd_rand> minstd_rand;
	/**
	 * Predefined
	 * [random_device](https://en.cppreference.com/w/cpp/numeric/random/random_device)
	 * engine.
	 */
	typedef Generator<std::random_device> random_device;
}}
#endif
