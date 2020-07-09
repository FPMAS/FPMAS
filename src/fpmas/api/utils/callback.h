#ifndef CALLBACK_API_H
#define CALLBACK_API_H

namespace fpmas { namespace api { namespace utils {
	template<typename... Args>
	class Callback {
		public:
			virtual void call(Args...) = 0;
			virtual ~Callback() {}
	};
}}}
#endif
