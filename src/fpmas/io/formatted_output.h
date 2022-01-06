#ifndef FPMAS_FORMATTED_OUTPUT_H
#define FPMAS_FORMATTED_OUTPUT_H

#include "output.h"

/** \file src/fpmas/io/formatted_output.h
 * Output implementation based on the output stream operator `<<`.
 */

namespace fpmas { namespace io {

	/**
	 * A
	 * [`FormattedOutputFunction`](https://en.cppreference.com/w/cpp/named_req/FormattedOutputFunction)
	 * based output.
	 *
	 * Data is serialized and sent directly through the
	 * ```
	 * std::ostream& operator<<(std::ostream&, const T&)
	 * ```
	 * specialization.
	 */
	template<typename T>
		class FormattedOutput : public OutputBase {
			private:
				Watcher<T> watcher;
				bool insert_new_line;
			public:
				/**
				 * FormattedOutput constructor.
				 *
				 * @param output_stream OutputStream to which data will be
				 * dumped.
				 * @param watcher Watcher instance used to gather data to dump().
				 * @param insert_new_line If true, an `std::endl` is inserted
				 * at the end of each dump()
				 */
				FormattedOutput(
						api::io::OutputStream& output_stream,
						Watcher<T> watcher,
						bool insert_new_line = false
						) :
					OutputBase(output_stream),
					watcher(watcher),
					insert_new_line(insert_new_line) {
				}

				/**
				 * Calls the `watcher` and send the result to the `output_stream`
				 * using the `<<` operator on `output_stream.get()`.
				 *
				 * If `insert_new_line` is true, and `std::endl` is also
				 * inserted.
				 */
				void dump() override {
					output_stream.get() << watcher();
					if(insert_new_line)
						output_stream.get() << std::endl;
				}
		};
}}
#endif
