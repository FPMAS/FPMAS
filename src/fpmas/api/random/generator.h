#ifndef FPMAS_RANDOM_GENERATOR_API_H
#define FPMAS_RANDOM_GENERATOR_API_H

/** \file src/fpmas/api/random/generator.h
 * Random Generator API
 */

#include <cstdint>

namespace fpmas { namespace api { namespace random {
	/**
	 * A virtual wrapper for random generators. It is a relaxed definition of
	 * the [C++ UniformRandomBitGenerator named
	 * requirement](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator),
	 * that can still be used in predefined random definitions.
	 */
	//TODO v2: template this interface so Generator really satisfies
	//UniformRandomBitGenerator?
	class Generator {
		public:
			/**
			 * Integer type used by the generator.
			 */
			typedef std::uint_fast64_t result_type;

			/**
			 * Minimum value that can be generated.
			 *
			 * @return min value
			 */
			virtual result_type min() = 0;
			/**
			 * Maximum value that can be generated.
			 *
			 * @return max value
			 */
			virtual result_type max() = 0;

			/**
			 * Returns a randomly generated value in [min(), max()].
			 *
			 * @return random integer
			 */
			virtual result_type operator()() = 0;

			virtual ~Generator() {}
	};
}}}
#endif
