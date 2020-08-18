#ifndef FPMAS_CALLBACK_H
#define FPMAS_CALLBACK_H

/** \file src/fpmas/utils/callback.h
 * Generic Callback implementation.
 */

#include "fpmas/api/utils/callback.h"

namespace fpmas { namespace utils {
	/**
	 * A Callback that does not perform any operation.
	 *
	 * Might be used as a default Callback in some situations.
	 */
	template<typename... Args>
	class VoidCallback : public api::utils::Callback<Args...> {
		public:
			void call(Args...) override {}
	};
}}
#endif
