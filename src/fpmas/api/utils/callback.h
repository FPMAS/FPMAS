#ifndef FPMAS_CALLBACK_API_H
#define FPMAS_CALLBACK_API_H

/** \file src/fpmas/api/utils/callback.h
 * Callback API
 */

namespace fpmas { namespace api { namespace utils {

	/**
	 * Generic callback API.
	 *
	 * @tparam Args Callback arguments
	 */
	template<typename... Args>
	class Callback {
		public:
			/**
			 * Calls the callback.
			 *
			 * @param args callback arguments
			 */
			virtual void call(Args... args) = 0;
			virtual ~Callback() {}
	};

	/**
	 * Event handling related Callback specialization.
	 *
	 * @tparam Event type of event handled by this callback
	 */
	template<typename Event>
		struct EventCallback : public Callback<const Event&> {
			/**
			 * Type of event handled by this callback.
			 */
			typedef Event EventType;
		};
}}}
#endif
