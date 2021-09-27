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
			 * Returns an executable \Job that might be scheduled with a
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
	/*
	 * Note: operator<< wrappers return std::ostream& instead of OutputStream&.
	 * This does not make much difference in terms of user experience, but is
	 * useful in the case of the DynamicFileOutput, where get() systematically
	 * reopens the file, what discard content if openmode is out.
	 * So for example, if OutputStream& is returned:
	 * ```
	 * dynamic_file_output << "hello" << "world";
	 * ```
	 * would print "world" to the file, since the second << operator call
	 * destroys the file.
	 * But, if std::ostream& is returned, only the first operator destroys the
	 * file so "helloworld" is printed.
	 *
	 * However, in the following example:
	 * ```
	 * dynamic_file_output << "hello";
	 * dynamic_file_output << "world";
	 * ```
	 * The file contains "world" since it is destroyed at the second call, what
	 * is however a relatively consistent behavior.
	 */
	struct OutputStream {
		/**
		 * Returns a reference to an std::ostream.
		 *
		 * @return output stream
		 */
		virtual std::ostream& get() = 0;

		/**
		 * Allows C++ standard output I/O manipulators (such as std::endl) to
		 * work with OutputStreams.
		 */
		std::ostream& operator<<(std::ostream& (*func)(std::ostream&)) {
			return this->get() << func;
		}

		virtual ~OutputStream() {}
	};

	/**
	 * Wrapper around `std::ostream& operator<<(std::ostream&, T data)`.
	 *
	 * Equivalent to `output.get() << data`.
	 */
	template<typename T>
		std::ostream& operator<<(OutputStream& output, const T& data) {
			return output.get() << data;
		}

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
