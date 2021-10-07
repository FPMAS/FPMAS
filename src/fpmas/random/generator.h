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
	 * A partial api::random::Generator implementation that meets the
	 * requirements of the
	 * [_UniformRandomBitGenerator_](https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator).
	 *
	 * @tparam Generator_t a predefined standard _UniformRandomBitGenerator_
	 */
	template<typename Generator_t>
		class UniformRandomBitGenerator: public api::random::Generator<typename Generator_t::result_type> {
			protected:
			/**
			 * Internal `Generator_t` instance.
			 */
			Generator_t gen;
			
			public:
			/**
			 * Integer type used by the generator.
			 */
			typedef typename Generator_t::result_type result_type;

			/**
			 * Minimum value that can be generated.
			 *
			 * @return min value
			 */
			static constexpr result_type min() {
				return Generator_t::min();
			}

			/**
			 * Maximum value that can be generated.
			 *
			 * @return max value
			 */
			static constexpr result_type max() {
				return Generator_t::max();
			}

			/**
			 * Returns a randomly generated value in [min(), max()].
			 *
			 * @return random integer
			 */
			result_type operator()() override {
				return gen();
			}
		};
	/**
	 * api::random::Generator implementation.
	 *
	 * Any specialization of this class meets the requirements of
	 * _UniformRandomBitGenerator_.
	 * 
	 * @tparam Generator_t cpp standard random number generator
	 */
	template<typename Generator_t>
		class Generator : public UniformRandomBitGenerator<Generator_t> {
			public:
				/**
				 * Integer type used by the generator.
				 */
				typedef typename UniformRandomBitGenerator<Generator_t>::result_type
					result_type;

				/**
				 * Default constructor.
				 */
				Generator() {}
				/**
				 * Initializes the Generator with the provided seed.
				 *
				 * @param seed random seed
				 */
				Generator(result_type seed) {
					this->seed(seed);
				}


				void seed(result_type seed) override {
					this->gen.seed(seed);
				}
		};

	/**
	 * std::random_device Generator specialization.
	 *
	 * Seeding this genererator has **no effect** but methods are still define
	 * to preserve API compatibility (Generator<std::random_device> is a
	 * concrete api::random::Generator implementation).
	 */
	template<>
		class Generator<std::random_device> : public UniformRandomBitGenerator<std::random_device> {
			public:
				/**
				 * Generator default constructor.
				 */
				Generator() {
				}
				/**
				 * Defines a constructor that accept a seed, but has no effect.
				 */
				Generator(std::random_device::result_type) {
				}

				/**
				 * Seed method: no effect in this case.
				 */
				void seed(std::random_device::result_type) override {
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
	 * More particularly, the generator internally uses a `Generator_t`
	 * instance, that must satisfy _UniformRandomBitGenerator_, seeded
	 * differently according to the rank of the process on which the generator
	 * is used. Since MPI communications are required to do so, fpmas::init()
	 * **must** be called before the first use of the generator. However, it
	 * is safe to instantiate the generator before fpmas::init() is called.
	 * This allows to declare a DistributedGenerator as a static variable.
	 *
	 * This class meets the requirements of _UniformRandomBitGenerator_.
	 *
	 * @tparam Generator_t type of the local random number generator used on
	 * each process
	 */
	template<typename Generator_t = mt19937_64>
		class DistributedGenerator : public api::random::Generator<typename Generator_t::result_type> {
			private:
				Generator_t local_generator;
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
					local_generator.seed(local_seed);
					_init = true;
				}

			public:
				/**
				 * Integer type used by the generator.
				 */
				typedef typename Generator_t::result_type result_type;

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

				void seed(result_type seed) override {
					seeder.seed(seed);
					init();
				}

				/**
				 * Minimum value that can be generated.
				 *
				 * @return min value
				 */
				static constexpr result_type min() {
					return Generator_t::min();
				}

				/**
				 * Maximum value that can be generated.
				 *
				 * @return max value
				 */
				static constexpr result_type max() {
					return Generator_t::max();
				}
		};

	template<>
		class DistributedGenerator<random_device> : public random_device {
		};

}}
#endif
