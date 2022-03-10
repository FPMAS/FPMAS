#ifndef FPMAS_FUNCTIONAL_H
#define FPMAS_FUNCTIONAL_H

/** \file src/fpmas/utils/functional.h
 * Utility function objects.
 */

#include <utility>
#include <functional>
#include <vector>
#include <unordered_map>
#include <map>

namespace fpmas { namespace utils {

	namespace detail {
		template<typename Container>
			struct Concat {
			};

		template<typename T>
			struct Concat<std::vector<T>> {
				static std::vector<T> concat(std::vector<T>& init, const std::vector<T>& c) {
					init.insert(init.end(), c.begin(), c.end());
					return std::move(init);
				}
			};

		template<typename K, typename T, typename Comp, typename Alloc>
			struct Concat<std::map<K, T, Comp, Alloc>> {
				static std::map<K, T, Comp, Alloc> concat(
						std::map<K, T, Comp, Alloc>& init,
						const std::map<K, T, Comp, Alloc>& c
						) {
					init.insert(c.begin(), c.end());
					return std::move(init);
				}
			};

		template<typename K, typename T, typename Hash, typename KeyEq, typename Alloc>
			struct Concat<std::unordered_map<K, T, Hash, KeyEq, Alloc>> {
				static std::unordered_map<K, T, Hash, KeyEq, Alloc> concat(
						std::unordered_map<K, T, Hash, KeyEq, Alloc>& init,
						const std::unordered_map<K, T, Hash, KeyEq, Alloc>& c
						) {
					init.insert(c.begin(), c.end());
					return std::move(init);
				}
			};
	}

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
				return detail::Concat<Container>::concat(init, c);
			}
	};

	/**
	 * A function object that can be used to compute the maximum between two
	 * values.
	 */
	template<typename T, typename Compare = std::less<T>>
		struct Max {
			/**
			 * Calls [std::max(a, b,
			 * Compare())](https://en.cppreference.com/w/cpp/algorithm/max).
			 */
			T operator()(const T& a, const T& b) {
				return std::max(a, b, Compare());
			}
		};
	/**
	 * A function object that can be used to compute the minimum between two
	 * values.
	 */
	template<typename T, typename Compare = std::less<T>>
		struct Min {
			/**
			 * Calls [std::min(a, b,
			 * Compare())](https://en.cppreference.com/w/cpp/algorithm/min).
			 */
			T operator()(const T& a, const T& b) {
				return std::min(a, b, Compare());
			}
		};
}}
#endif
