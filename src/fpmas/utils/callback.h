#ifndef CALLBACK_H
#define CALLBACK_H

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
