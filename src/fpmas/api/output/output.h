#ifndef FPMAS_OUTPUT_API_H
#define FPMAS_OUTPUT_API_H

#include "fpmas/api/scheduler/scheduler.h"

/** \file src/fpmas/api/output/output.h
 *
 * Program outputs relative APIs.
 */

namespace fpmas { namespace api { namespace output {

	/**
	 * A generic output API.
	 */
	class Output {
		public:
			/**
			 * Writes data to the output.
			 */
			virtual void dump() = 0;
			/**
			 * Returns an executable \Job that might be sheduled with a
			 * scheduler::Scheduler.
			 *
			 * The execution of the \Job calls dump() exactly once.
			 *
			 * @return executable job
			 */
			virtual const scheduler::Job& job() = 0;

			virtual ~Output() {}
	};
}}}
#endif
