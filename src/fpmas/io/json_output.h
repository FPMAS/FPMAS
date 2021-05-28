#ifndef FPMAS_JSON_OUTPUT_H
#define FPMAS_JSON_OUTPUT_H

#include "output.h"
namespace fpmas { namespace io {

	/**
	 * An fpmas::api::io::Output implementation that dumps data of a single
	 * Watcher as JSON to the provided OutputStream.
	 *
	 * @tparam T Data type
	 */
	template<typename T>
	class JsonOutput : public OutputBase {
		private:
			Watcher<T> watcher;

		public:
			/**
			 * JsonOutput constructor.
			 *
			 * @param output_stream OutputStream to which JSON data will be
			 * dumped.
			 * @param watcher Watcher instance used to gather data to dump().
			 */
			JsonOutput(api::io::OutputStream& output_stream, Watcher<T> watcher)
				: OutputBase(output_stream), watcher(watcher) {
				}

			/**
			 * 1. Gathers data with a `watcher()` call.
			 * 2. Serializes the data instance (of type T) using the `nlohmann`
			 * serialization process.
			 * 3. Dumps JSON data to the `output_stream`.
			 */
			void dump() override {
				nlohmann::json j = watcher();
				output_stream.get() << j.dump(1);
			}
	};
}}
#endif
