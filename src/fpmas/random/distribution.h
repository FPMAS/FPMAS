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
	 * @tparam Distribution_t predefined C++ distribution, that must satisfy
	 * _RandomNumberDistribution_.
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

			/**
			 * Generates a random value from the input generator, according to
			 * the implemented random distribution.
			 *
			 * @tparam Generator_t Random number generator (must satisfy
			 * _UniformRandomBitGenerator_)
			 *
			 * @param generator random number generator
			 * @return random value
			 */
			template<typename Generator_t>
				result_type operator()(Generator_t& generator) {
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
	 * [BernoulliDistribution](https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution).
	 */
	typedef Distribution<std::bernoulli_distribution> BernoulliDistribution;

	/**
	 * Predefined
	 * [BinomialDistribution](https://en.cppreference.com/w/cpp/numeric/random/binomial_distribution).
	 */
	template<typename IntType = unsigned int>
		using BinomialDistribution = Distribution<std::binomial_distribution<IntType>>;

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

	/**
	 * A constant "random" distribution that always return the given value, with
	 * a probability `P(value)=1`.
	 *
	 * This is notably useful for graph builders such as
	 * fpmas::graph::DistributedClusteredGraphBuilder, when the outgoing edges
	 * count of each node must be constant and not random.
	 */
	template<typename T>
	class ConstantDistribution : public api::random::Distribution<T> {
		private:
			T value;

		public:
			/**
			 * Type of generated values.
			 */
			typedef T result_type;

			/**
			 * Generic ConstantDistribution constructor.
			 *
			 * @param value constant value of the distribution
			 */
			ConstantDistribution(const T& value) : value(value) {}

			/**
			 * Always returns the same value.
			 *
			 * @tparam Generator_t Random number generator
			 *
			 * @return constant value
			 */
			template<typename Generator_t>
				result_type operator()(Generator_t&) {
					return value;
				}

			/**
			 * \copydoc fpmas::api::random::Distribution::min()
			 */
			result_type min() const override {
				return value;
			}

			/**
			 * \copydoc fpmas::api::random::Distribution::max()
			 */
			result_type max() const override {
				return value;
			}
	};
}}

#endif
