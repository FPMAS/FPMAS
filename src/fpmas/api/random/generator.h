#ifndef FPMAS_RANDOM_GENERATOR_API_H
#define FPMAS_RANDOM_GENERATOR_API_H

#include <cstdint>

namespace fpmas { namespace api { namespace random {
	class Generator {
		public:
			typedef std::uint_fast64_t result_type;

			virtual result_type min() = 0;
			virtual result_type max() = 0;

			virtual result_type operator()() = 0;
	};
}}}
#endif
