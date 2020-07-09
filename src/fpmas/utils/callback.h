#ifndef CALLBACK_H
#define CALLBACK_H

#include "fpmas/api/utils/callback.h"

namespace fpmas { namespace utils {
	template<typename... Args>
	class VoidCallback : public api::utils::Callback<Args...> {
		public:
			void call(Args...) override {}
	};
}}
#endif
