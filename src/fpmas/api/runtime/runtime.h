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
			 * The Runtime is currently based on a unit time step. In consequence,
			 * the Runtime will execute jobs at dates `start`, `start+1` and
			 * `start+n` for any integer n such that `start+n < end`.
			 *
			 * At the end of the execution, currentDate() is reinitialized to
			 * 0.
			 *
			 * @param start start date
			 * @param end end date
			 */
			virtual void run(Date start, Date end) = 0;
			/**
			 * Runs jobs from currentDate() to end, excluded.
			 *
			 * Equivalent to
			 * ```cpp
			 * this->run(this->currentDate(), end);
			 * ```
			 *
			 * @param end end date
			 *
			 * @see run(Date, Date)
			 */
			virtual void run(Date end) = 0;

			/**
			 * Executes the provided `job`.
			 *
			 * Tasks of the job (excluding scheduler::Job::getBeginTask() and
			 * scheduler::Job::getEndTask()) are executed in a random order.
			 *
			 * @param job job to execute
			 */
			virtual void execute(const scheduler::Job& job) = 0;

			/**
			 * Sequentially executes the jobs contained in the provided
			 * `job_list`.
			 *
			 * @param job_list list of jobs to execute sequentially
			 */
			virtual void execute(const scheduler::JobList& job_list) = 0;

			/**
			 * Current internal date of the runtime.
			 *
			 * The current date is initialized to 0 when a Runtime is built.
			 *
			 * @return current date of this Runtime
			 */
			virtual Date currentDate() const = 0;

			/**
			 * Sets the internal date of this Runtime, so that the next call to
			 * run(Date) will start to run jobs from `date`.
			 *
			 * @param date new date
			 */
			virtual void setCurrentDate(Date date) = 0;

			virtual ~Runtime() {}
	};
}}}
#endif
