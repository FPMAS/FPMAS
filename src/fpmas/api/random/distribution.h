#ifndef FPMAS_RANDOM_DISTRIBUTION_API_H
#define FPMAS_RANDOM_DISTRIBUTION_API_H

#include "generator.h"

namespace fpmas { namespace api { namespace random {

	template<typename T>
	class Distribution {
		public:
			virtual T operator()(Generator& generator) = 0;
	};
}}}
#endif
