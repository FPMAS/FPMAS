#ifndef FPMAS_RUNTIME_API_H
#define FPMAS_RUNTIME_API_H

/** \file src/fpmas/api/runtime/runtime.h
 * Runtime API
 */

#include "fpmas/api/scheduler/scheduler.h"

namespace fpmas { namespace api { namespace runtime {
	using api::scheduler::Date;

	/**
	 * Runtime API.
	 *
	 * The purpose of a Runtime is to execute jobs scheduled in a Scheduler.
	 */
	class Runtime {
		public:
			/**
			 * Runs jobs from start, included, to end, excluded.
			 *
			 * The Runtime is currently base on a unit time step. In consequence,
			 * the Runtime will execute jobs at dates `start`, `start+1` and
			 * `start+n` for any integer n such that `start+n < end`.
			 *
			 * @param start start date
			 * @param end end date
			 */
			virtual void run(Date start, Date end) = 0;
			/**
			 * Alias for `run(0, end)`.
			 *
			 * @param end end date
			 */
			virtual void run(Date end) = 0;
			/**
			 * Current internal date of the runtime.
			 */
			virtual Date currentDate() const = 0;
	};
}}}
#endif
