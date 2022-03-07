#ifndef FPMAS_RANDOM_GENERATOR_API_H
#define FPMAS_RANDOM_GENERATOR_API_H

/** \file src/fpmas/api/random/generator.h
 * Random Generator API
 */

#include <cstdint>

namespace fpmas { namespace api { namespace random {
	/**
	 * A virtual wrapper for random generators.
	 *
	 * Any implementation of this interface must meet the requirements of
	 * [_UniformRandomBitGenerator_](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator).
	 *
	 * @tparam T generated integer type
	 */
	template<typename T>
		class Generator {
			public:
				/**
				 * Returns a randomly generated value in [min(), max()].
				 *
				 * @return random integer
				 */
				virtual T operator()() = 0;

				/**
				 * Seeds the random generator.
				 *
				 * @param seed seed value
				 */
				virtual void seed(T seed) = 0;

				/**
				 * Advances the generator internal state as if operator() was
				 * called z times.
				 *
				 * @param z number of values to discard
				 */
				virtual void discard(unsigned long long z) = 0;

				virtual ~Generator() {}
		};
}}}
#endif
