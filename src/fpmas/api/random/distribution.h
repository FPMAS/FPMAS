#ifndef FPMAS_RANDOM_DISTRIBUTION_API_H
#define FPMAS_RANDOM_DISTRIBUTION_API_H

/** \file src/fpmas/api/random/distribution.h
 * Random Distribution API
 */

#include "generator.h"

namespace fpmas { namespace api { namespace random {

	/**
	 * Defines a generic random distribution API.
	 *
	 * @tparam T type of generated random values
	 */
	template<typename T>
	class Distribution {
		public:
			/**
			 * Generates a random value from the input generator, according to
			 * the implemented random distribution.
			 *
			 * @param generator random number generator
			 * @return random value
			 */
			virtual T operator()(Generator& generator) = 0;
	};
}}}
#endif
