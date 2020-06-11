#ifndef CALLBACK_API_H
#define CALLBACK_API_H

namespace FPMAS::api::utils {
	/*
	 *class Callback {
	 *    public:
	 *        virtual void call() = 0;
	 *        virtual ~Callback() {}
	 *};
	 */

	template<typename... Args>
	class Callback {
		public:
			virtual void call(Args...) = 0;
			virtual ~Callback() {}
	};
}
#endif
