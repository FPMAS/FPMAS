#ifndef FPMAS_RANDOM_DISTRIBUTION_H
#define FPMAS_RANDOM_DISTRIBUTION_H

#include <random>
#include "fpmas/api/random/distribution.h"

namespace fpmas { namespace random {

	template<typename Distribution_t>
	class Distribution : public api::random::Distribution<typename Distribution_t::result_type> {
		private:
			Distribution_t distrib;

		public:
			typedef typename Distribution_t::result_type result_type;

			template<typename... Args>
				Distribution(Args... args) : distrib(args...) {}

			result_type operator()(api::random::Generator& generator) override {
				return distrib(generator);
			}
	};

	template<typename IntType = int>
		using UniformIntDistribution = Distribution<std::uniform_int_distribution<IntType>>;

	template<typename FloatType = double>
		using UniformRealDistribution = Distribution<std::uniform_real_distribution<FloatType>>;

	template<typename FloatType = double>
		using NormalDistribution = Distribution<std::normal_distribution<FloatType>>;

	template<typename IntType = int>
		using PoissonDistribution = Distribution<std::poisson_distribution<IntType>>;
}}

#endif
