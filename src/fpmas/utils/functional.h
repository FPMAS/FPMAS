#ifndef FPMAS_FUNCTIONAL_H
#define FPMAS_FUNCTIONAL_H

#include <utility>

namespace fpmas { namespace utils {
	/**
	 * A function object that can be used to concatenate two containers.
	 */
	struct Concat {
		/**
		 * Call operator.
		 *
		 * Elements of `c` are inserted at the end of `init`, that is finally returned.
		 *
		 * In order to avoid useless copies, `init` is moved upon return.
		 *
		 * @param init current container, in which items of `c` will be
		 * inserted.
		 * @param c items to add to `init`
		 * @return concatenation of `init` and `c`
		 *
		 * @tparam Container container type (automatically deduced in most
		 * contexts)
		 */
		template<typename Container>
			Container operator()(Container& init, const Container& c) const {
				init.insert(init.end(), c.begin(), c.end());
				return std::move(init);
			}
	};

}}
#endif
