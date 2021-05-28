#ifndef FPMAS_OUTPUT_API_H
#define FPMAS_OUTPUT_API_H

#include "fpmas/api/scheduler/scheduler.h"

/** \file src/fpmas/api/io/output.h
 *
 * Program outputs relative APIs.
 */

namespace fpmas { namespace api { namespace io {

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

	/**
	 * A generic `std::ostream` wrapper.
	 *
	 * This allows to implement dynamic output streams, with a generic
	 * interface.
	 *
	 * For example, the method get() might open and return a reference to a
	 * different output file depending on the current simulation step, while
	 * outputs are always performed to OutputStream::get().
	 */
	struct OutputStream {
		/**
		 * Returns a reference to an abstract std::ostream.
		 *
		 * @return output stream
		 */
		virtual std::ostream& get() = 0;

		virtual ~OutputStream() {}
	};

	/**
	 * Generic Watcher interface.
	 *
	 * A Watcher is a callable object that dynamically returns some underlying
	 * data.
	 */
	template<typename T>
		using Watcher = std::function<T()>;
}}}
#endif
