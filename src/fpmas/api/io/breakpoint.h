#ifndef FPMAS_BREAKPOINT_API_H
#define FPMAS_BREAKPOINT_API_H

#include <istream>
#include <ostream>

namespace fpmas { namespace api { namespace io {
	/**
	 * Breakpoint API.
	 *
	 * A breakpoint offers features to save and load the state of any instance
	 * of type `T` to and from C++ standard I/O streams
	 * ([std::fstream](https://en.cppreference.com/w/cpp/io/basic_fstream) and
	 * [std::stringstream](https://en.cppreference.com/w/cpp/io/basic_stringstream)
	 * can notably be useful).
	 *
	 * @tparam T type of data to save
	 */
	template<typename T>
		class Breakpoint {
			public:
				/**
				 * Dumps `data` to the specified `stream`.
				 *
				 * @param stream output stream
				 * @param object object to save
				 */
				virtual void dump(std::ostream& stream, const T& object) = 0;

				/**
				 * Loads data from the specified `stream`, and loads it "into"
				 * the specified `object` instance.
				 *
				 * So for example, if `std::vector<int>` is used for `T`, there
				 * is no guarantee that the specified `data` will be
				 * effectively cleared before data is loaded into it. Such
				 * details are implementation defined, potentially depending on
				 * the type `T`.
				 *
				 * @param stream input stream
				 * @param object T instance in which read data will be loaded
				 */
				virtual void load(std::istream& stream, T& object) = 0;

				virtual ~Breakpoint() {}
		};
}}}
#endif
