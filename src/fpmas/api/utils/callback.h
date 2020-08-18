#ifndef FPMAS_CALLBACK_API_H
#define FPMAS_CALLBACK_API_H

/** \file src/fpmas/api/utils/callback.h
 * Callback API
 */

namespace fpmas { namespace api { namespace utils {
	template<typename... Args>
	class Callback {
		public:
			virtual void call(Args...) = 0;
			virtual ~Callback() {}
	};
}}}
#endif
