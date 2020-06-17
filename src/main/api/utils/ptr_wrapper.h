#ifndef PTR_WRAPPER_API_H
#define PTR_WRAPPER_API_H

namespace FPMAS::api::utils {

	template<typename VirtualType>
	class VirtualPtrWrapper {
		protected:
			 VirtualType* virtual_type_ptr;
		public:
			 VirtualPtrWrapper()
				 : virtual_type_ptr(nullptr) {};
			 VirtualPtrWrapper(VirtualType* virtual_type_ptr)
				 : virtual_type_ptr(virtual_type_ptr) {}

			 VirtualType* get() {
				 return virtual_type_ptr;
			 }

			 const VirtualType* get() const {
				 return virtual_type_ptr;
			 }

			 VirtualType* release() {
				 VirtualType* ptr = virtual_type_ptr;
				 virtual_type_ptr = nullptr;
				 return ptr;
			 }

			 VirtualType* operator->() {
				 return virtual_type_ptr;
			 }

			 const VirtualType* operator->() const {
				 return virtual_type_ptr;
			 }

			 operator VirtualType*() {
				 return virtual_type_ptr;
			 }
			 operator const VirtualType*() const {
				 return virtual_type_ptr;
			 }
	};
}
#endif
