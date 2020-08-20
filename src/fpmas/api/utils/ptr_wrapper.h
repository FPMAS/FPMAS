#ifndef FPMAS_PTR_WRAPPER_API_H
#define FPMAS_PTR_WRAPPER_API_H

/** \file src/fpmas/api/utils/ptr_wrapper.h
 * PtrWrapper implementation.
 */

namespace fpmas { namespace api { namespace utils {

	/**
	 * A trivial pointer wrapper helper class.
	 *
	 * Such wrappers are notably used to allow JSON serialization of pointers
	 * with the nlohmann library, since using directly a pointer as type raised
	 * some errors.
	 *
	 * It also maked pointer usage as data type in api::graph::DistributedGraph
	 * easier.
	 */
	template<typename T>
	class PtrWrapper {
		protected:
			/**
			 * Internal pointer.
			 */
			 T* ptr;
		public:
			 /**
			  * PtrWrapper default constructor.
			  *
			  * The internal pointer is initialized to `nullptr`.
			  */
			 PtrWrapper()
				 : ptr(nullptr) {};
			 /**
			  * PtrWrapper constructor.
			  *
			  * @param ptr pointer to wrap
			  */
			 PtrWrapper(T* ptr)
				 : ptr(ptr) {}

			 /**
			  * Gets the internal pointer.
			  *
			  * @return internal pointer
			  */
			 T* get() {
				 return ptr;
			 }

			 /**
			  * \copydoc get()
			  */
			 const T* get() const {
				 return ptr;
			 }

			 /**
			  * Returns the internal pointer and sets it to `nullptr`.
			  *
			  * @return internal pointer
			  */
			 T* release() {
				 T* ptr = ptr;
				 ptr = nullptr;
				 return ptr;
			 }

			 /**
			  * Member of pointer operator.
			  *
			  * @return internal pointer
			  */
			 T* operator->() {
				 return ptr;
			 }

			 /**
			  * \copydoc operator->()
			  */
			 const T* operator->() const {
				 return ptr;
			 }

			 /**
			  * Indirection operator.
			  *
			  * @return reference to the object to which the internal pointer
			  * points
			  */
			 T& operator*() {
				 return *ptr;
			 }

			 /**
			  * \copydoc operator*()
			  */
			 const T& operator*() const {
				 return *ptr;
			 }

			 /**
			  * Implicit pointer conversion operator.
			  */
			 operator T*() {
				 return ptr;
			 }

			 /**
			  * Implicit pointer conversion operator.
			  */
			 operator const T*() const {
				 return ptr;
			 }
	};
}}}
#endif
