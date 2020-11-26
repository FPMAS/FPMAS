#ifndef FPMAS_RANDOM_GENERATOR_H
#define FPMAS_RANDOM_GENERATOR_H

/** \file src/fpmas/random/generator.h
 * Random Generator implementation.
 */

#include <cstdint>
#include <random>
#include "fpmas/api/random/generator.h"
#include "fpmas/communication/communication.h"

namespace fpmas { namespace random {

	template<typename> class DistributedGenerator;
	/**
	 * api::random::Generator implementation.
	 */
	template<typename Generator_t>
	class Generator : public api::random::Generator {
		friend DistributedGenerator<Generator<Generator_t>>;
		protected:
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

	template<typename GeneratorType = mt19937_64>
		class DistributedGenerator : public api::random::Generator {
			private:
				GeneratorType local_generator;
				std::mt19937 seeder;

				bool _init = false;
				void init() {
					int rank = communication::MpiCommunicator::WORLD.getRank();
					result_type local_seed = seeder() + rank;
					local_generator.gen.seed(local_seed);
					_init = true;
				}

			public:
				DistributedGenerator() {}
				DistributedGenerator(result_type seed)
					: seeder(seed) {}

				result_type operator()() override {
					if(!_init)
						init();
					return local_generator();
				}

				result_type min() override {
					return local_generator.min();
				}

				result_type max() override {
					return local_generator.max();
				}
		};

	template<>
		class DistributedGenerator<random_device> : public random_device {
		};

}}
#endif
