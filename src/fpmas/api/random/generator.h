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
	 *
	 * @tparam T generated integer type
	 */
	//TODO v2: refactor random api?
	template<typename T>
		class Generator {
			public:
				/**
				 * Returns a randomly generated value in [min(), max()].
				 *
				 * @return random integer
				 */
				virtual T operator()() = 0;

				virtual ~Generator() {}
		};
}}}
#endif
