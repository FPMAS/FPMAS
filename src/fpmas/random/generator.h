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


#ifndef DOXYGEN_BUILD
	template<typename> class DistributedGenerator;
#endif

	/**
	 * api::random::Generator implementation.
	 */
	template<typename Generator_t>
		class Generator : public api::random::Generator {
			friend DistributedGenerator<Generator<Generator_t>>;
			protected:
			/**
			 * Internal `Generator_t` instance.
			 */
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

	/**
	 * Distributed api::random::Generator implementation.
	 *
	 * The generator is assumed to produce pseudo-random and deterministic,
	 * that are still different on each process.
	 *
	 * More particularly, the generator internally uses a `GeneratorType`
	 * instance, that might be any api::random::Generator implementation, that
	 * is seeded differently according to the rank of the process on which the
	 * generator is used. Since MPI communications are required to do so,
	 * fpmas::init() **must** be called before the first use of the generator.
	 * However, it is safe to instantiate the generator before fpmas::init() is
	 * called. This allows to declare a DistributedGenerator as a static
	 * variable.
	 */
	template<typename GeneratorType = mt19937_64>
		class DistributedGenerator : public api::random::Generator {
			private:
				GeneratorType local_generator;
				std::mt19937 seeder;

				bool _init = false;
				/**
				 * Initializes the DistributedGenerator. Called the first time
				 * the generator is called.
				 */
				void init() {
					int rank = communication::WORLD.getRank();
					// Generates a different seed on each process
					result_type local_seed = seeder() + rank;
					// Seeds the local generator with the generated seed
					local_generator.gen.seed(local_seed);
					_init = true;
				}

			public:
				/**
				 * Default DistributedGenerator constructor from a default
				 * seed.
				 */
				DistributedGenerator() {}
				/**
				 * DistributedGenerator constructor.
				 *
				 * The DistributedGenerator is initialized using the provided
				 * seed.
				 *
				 * More particularly, the `seed` is used to seed an internal
				 * pseudo-random number generator that is itself used to seed
				 * the internal `GeneratorType` instance: it is still ensured
				 * that this last generator is seeded differently on each
				 * process.
				 *
				 * @param seed DistributedGenerator seed
				 */
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
