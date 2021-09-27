#include "fpmas/api/utils/ptr_wrapper.h"
#include "fpmas/api/model/model.h"

namespace fpmas { namespace utils {

	namespace detail {
		/**
		 * fpmas::utils::ptr_wrapper_cast implementation.
		 *
		 * A class is used to allow partial template specialization.
		 *
		 * @tparam T1 target type
		 * @tparam T2 source type
		 */
		template<typename T1, typename T2>
			struct PtrWrapperCastImpl {
				/**
				 * @see fpmas::utils::ptr_wrapper_cast
				 */
				static api::utils::PtrWrapper<T1> cast(T2* ptr) {
					return {dynamic_cast<T1*>(ptr)};
				}
			};

		/**
		 * PtrWrapperCastImpl specialization when `T1 == T2`.
		 *
		 * In this case, no `dynamic_cast` is used, so the source pointer just
		 * needs to be wrapped in a PtrWrapper instance.
		 */
		template<typename T>
			struct PtrWrapperCastImpl<T, T> {
				/**
				 * Wraps `ptr` in an fpmas::api::utils::PtrWrapper instance.
				 */
				static api::utils::PtrWrapper<T> cast(T* ptr) {
					return {ptr};
				}
			};
	}

	/**
	 * `ptr` is converted to `T1` using a `dynamic_cast` operation,
	 * and the result is wrapped and returned in an
	 * fpmas::api::utils::PtrWrapper instance.
	 *
	 * If `T1==T2`, no `dynamic_cast` is performed and the result is just
	 * wrapped in an fpmas::api::utils::PtrWrapper instance.
	 *
	 * @param ptr pointer to cast
	 * @return cast result wrapped in a PtrWrapper
	 *
	 * @tparam T1 target type
	 * @tparam T2 source type
	 */
	template<typename T1, typename T2>
		api::utils::PtrWrapper<T1> ptr_wrapper_cast(T2* ptr) {
			return detail::PtrWrapperCastImpl<T1, T2>::cast(ptr);
		}

	/**
	 * Applies ptr_wrapper_cast to all items of the input vector and returns a
	 * new fpmas::api::utils::PtrWrapper vector.
	 *
	 * @param vec vector containing pointers to cast and wrap
	 * @return cast and wrapped items
	 *
	 * @tparam T1 items target type
	 * @tparam T2 items source type
	 */
	template<typename T1, typename T2>
		std::vector<api::utils::PtrWrapper<T1>> ptr_wrapper_cast(std::vector<T2*> vec) {
			std::vector<api::utils::PtrWrapper<T1>> _vec;
			for(auto agent : vec)
				_vec.push_back(ptr_wrapper_cast<T1, T2>(agent));
			return _vec;
		}
}}
