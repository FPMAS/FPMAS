#ifndef FPMAS_RANDOM_GENERATOR_H
#define FPMAS_RANDOM_GENERATOR_H

/** \file src/fpmas/random/generator.h
 * Random Generator implementation.
 */

#include <cstdint>
#include <random>
#include <sstream>
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
			using typename api::random::Generator<typename Generator_t::result_type>::result_type;

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

	template<typename Generator_t>
		class Generator;

	/**
	 * Serializes the specified generator to an output stream.
	 *
	 * The implementation is directly based on the C++11 standard
	 * [RandomNumberEngine
	 * operator<<](https://en.cppreference.com/w/cpp/named_req/RandomNumberEngine)
	 * specification.
	 */
	template<typename Gen, class CharT, class Traits>
		std::basic_ostream<CharT, Traits>& operator<<(
				std::basic_ostream<CharT,Traits>& ost, const Generator<Gen>& g ) {
			return ost << g.gen;
		}

	/**
	 * Unserializes a generator from an input stream.
	 *
	 * The produced generator continues to generate its random number sequence
	 * from the state it had when serialized.
	 *
	 * The implementation is directly based on the C++11 standard
	 * [RandomNumberEngine
	 * operator>>](https://en.cppreference.com/w/cpp/named_req/RandomNumberEngine)
	 * specification.
	 */
	template<typename Gen, class CharT, class Traits>
		std::basic_istream<CharT,Traits>&
		operator>>(std::basic_istream<CharT,Traits>& ost, Generator<Gen>& g ) {
			return ost >> g.gen;
		}

	/**
	 * api::random::Generator implementation.
	 *
	 * Any specialization of this class meets the requirements of
	 * _UniformRandomBitGenerator_.
	 * 
	 * @see fpmas::random::operator<<()
	 * @see fpmas::random::operator>>()
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

				void discard(unsigned long long z) override {
					this->gen.discard(z);
				}

				/**
				 * Returns true iff the internal state of the generator `g` is
				 * equal to the state of this generator, i.e. the two
				 * generators are guaranteed to generate the same random
				 * numbers sequence.
				 */
				bool operator==(const Generator<Generator_t>& g) const {
					return this->gen == g.gen;
				};

				/**
				 * Equivalent to `!(*this == g)`.
				 */
				bool operator!=(const Generator<Generator_t>& g) const {
					return this->gen != g.gen;
				};

				template<typename Gen, class CharT, class Traits>
					friend std::basic_ostream<CharT,Traits>&
					operator<<(std::basic_ostream<CharT,Traits>& ost, const Generator<Gen>& g );

				template<typename Gen, class CharT, class Traits>
					friend std::basic_istream<CharT,Traits>&
					operator>>(std::basic_istream<CharT,Traits>& ost, Generator<Gen>& g );
		};

	/**
	 * random_device specialization of the generator output stream operator.
	 *
	 * Does not need to serialize anything.
	 */
	template<class CharT, class Traits>
		std::basic_ostream<CharT, Traits>& operator<<(
				std::basic_ostream<CharT,Traits>& ost, const Generator<std::random_device>&) {
			return ost;
		}

	/**
	 * random_device specialization of the generator input stream operator.
	 *
	 * Does not need to unserialize anything.
	 */
	template<class CharT, class Traits>
		std::basic_istream<CharT,Traits>&
		operator>>(std::basic_istream<CharT,Traits>& ost, Generator<std::random_device>& ) {
			return ost;
		}

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
				 * Generator<std::random_device> specialization copy
				 * constructor.
				 *
				 * Does not copy the internal std::random_device, since the
				 * next generated values in non-deterministic anyway.
				 */
				Generator(const Generator<std::random_device>&) {
				}

				/**
				 * Generator<std::random_device> specialization copy
				 * constructor.
				 *
				 * Does not move the internal std::random_device, since the
				 * next generated values in non-deterministic anyway.
				 */
				Generator(Generator<std::random_device>&&) {
				}

				/**
				 * Generator<std::random_device> specialization copy
				 * assignment operator.
				 *
				 * Does not copy assign the internal std::random_device, since
				 * the next generated values in non-deterministic anyway.
				 */
				Generator<std::random_device>& operator=(const Generator<std::random_device>&) {
					return *this;
				}

				/**
				 * Generator<std::random_device> specialization move
				 * assignment operator.
				 *
				 * Does not move assign the internal std::random_device, since
				 * the next generated values in non-deterministic anyway.
				 */
				Generator<std::random_device>& operator=(Generator<std::random_device>&&) {
					return *this;
				}

				/**
				 * No effect in the case of the random_device generator.
				 */
				void seed(std::random_device::result_type) override {
				}

				/**
				 * No effect in the case of the random_device generator.
				 */
				void discard(unsigned long long) override {
				}

				template<typename Gen, class CharT, class Traits>
					friend std::basic_ostream<CharT,Traits>&
					operator<<(std::basic_ostream<CharT,Traits>& ost, const Generator<Gen>& g );

				template<typename Gen, class CharT, class Traits>
					friend std::basic_istream<CharT,Traits>&
					operator>>(std::basic_istream<CharT,Traits>& ost, Generator<Gen>& g );

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

				void discard(unsigned long long z) override {
					if(!_init)
						init();
					local_generator.discard(z);
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

namespace nlohmann {
	/**
	 * Json serialization rules for fpmas::random::Generator.
	 */
	template<typename Generator_t>
		struct adl_serializer<fpmas::random::Generator<Generator_t>> {

			/**
			 * Serializes `gen` into `j` using the fpmas::random::operator<<().
			 *
			 * @param j output json
			 * @param gen input generator
			 */
			template<typename JsonType>
				static void to_json(JsonType& j, const fpmas::random::Generator<Generator_t>& gen) {
					std::stringstream str_buf;
					str_buf << gen;
					j = str_buf.str();
				}

			/**
			 * Unserializes `gen` from `j` using the fpmas::random::operator>>().
			 *
			 * @param j input json
			 * @param gen output generator
			 */
			template<typename JsonType>
				static void from_json(const JsonType& j, fpmas::random::Generator<Generator_t>& gen) {
					std::stringstream str_buf(j.template get<std::string>());
					str_buf >> gen;
				}
		};
}

namespace fpmas { namespace io { namespace datapack {
	/**
	 * ObjectPack serialization rules for fpmas::random::Generator.
	 */
	template<typename Generator_t>
		struct Serializer<fpmas::random::Generator<Generator_t>> {
			/**
			 * Serializes `gen` into a output string stream using the
			 * fpmas::random::operator<<(), and returns the size required to
			 * serialize the output string to `pack`.
			 *
			 * @param pack output ObjectPack
			 * @param gen input generator
			 */
			template<typename PackType>
				static std::size_t size(
						const PackType& pack,
						const fpmas::random::Generator<Generator_t>& gen) {
					std::ostringstream str;
					str << gen;
					return pack.size(str.str());
				}

			/**
			 * Serializes `gen` into `pack` using the fpmas::random::operator<<().
			 *
			 * @param pack output ObjectPack
			 * @param gen input generator
			 */
			template<typename PackType>
				static void to_datapack(PackType& pack, const fpmas::random::Generator<Generator_t>& gen) {
					std::ostringstream str;
					str<<gen;
					pack.template put(str.str());
				}

			/**
			 * Unserializes `gen` from `j` using the fpmas::random::operator<<().
			 *
			 * @param pack input ObjectPack
			 * @return unserialized generator
			 */
			template<typename PackType>
				static fpmas::random::Generator<Generator_t> from_datapack(
						const PackType& pack) {
					std::istringstream str(pack.template get<std::string>());
					fpmas::random::Generator<Generator_t> gen;
					str>>gen;
					return gen;
				}
		};
}}}
#endif
