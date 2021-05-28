#ifndef FPMAS_RANDOM_DISTRIBUTION_H
#define FPMAS_RANDOM_DISTRIBUTION_H

/** \file src/fpmas/random/distribution.h
 * Random Distribution implementation.
 */

#include <random>
#include "fpmas/api/random/distribution.h"

namespace fpmas { namespace random {

	/**
	 * Generic api::random::Distribution implementation.
	 *
	 * The specified template parameter can be any distribution that satisfy
	 * the requirements of the [C++ RandomNumberDistribution named
	 * requirement](https://en.cppreference.com/w/cpp/named_req/RandomNumberDistribution).
	 *
	 * @tparam predefined C++ distribution
	 */
	template<typename Distribution_t>
	class Distribution : public api::random::Distribution<typename Distribution_t::result_type> {
		private:
			Distribution_t distrib;

		public:
			/**
			 * Type of generated values.
			 */
			typedef typename Distribution_t::result_type result_type;

			/**
			 * Generic Distribution constructor.
			 *
			 * `Distribution_t` is assumed to be constructible from the
			 * specified arguments.
			 */
			template<typename... Args>
				Distribution(Args... args) : distrib(args...) {}

			result_type operator()(api::random::Generator& generator) override {
				return distrib(generator);
			}

			/**
			 * \copydoc fpmas::api::random::Distribution::min()
			 */
			result_type min() const override {
				return distrib.min();
			}

			/**
			 * \copydoc fpmas::api::random::Distribution::max()
			 */
			result_type max() const override {
				return distrib.max();
			}
	};

	/**
	 * Predefined
	 * [UniformIntDistribution](https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution).
	 */
	template<typename IntType = int>
		using UniformIntDistribution = Distribution<std::uniform_int_distribution<IntType>>;

	/**
	 * Predefined
	 * [UniformRealDistribution](https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution).
	 */
	template<typename FloatType = double>
		using UniformRealDistribution = Distribution<std::uniform_real_distribution<FloatType>>;

	/**
	 * Predefined
	 * [NormalDistribution](https://en.cppreference.com/w/cpp/numeric/random/normal_distribution).
	 */
	template<typename FloatType = double>
		using NormalDistribution = Distribution<std::normal_distribution<FloatType>>;

	/**
	 * Predefined
	 * [PoissonDistribution](https://en.cppreference.com/w/cpp/numeric/random/poisson_distribution).
	 */
	template<typename IntType = int>
		using PoissonDistribution = Distribution<std::poisson_distribution<IntType>>;
}}

#endif
