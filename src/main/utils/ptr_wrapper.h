#ifndef PTR_WRAPPER_H
#define PTR_WRAPPER_H

namespace FPMAS::utils {

	template<typename VirtualType>
	class VirtualPtrWrapper {
		private:
			 VirtualType* virtual_type_ptr;
		public:
			 VirtualPtrWrapper(VirtualType* virtual_type_ptr)
				 : virtual_type_ptr(virtual_type_ptr) {}

			 VirtualType* get() {
				 return virtual_type_ptr;
			 }

			 const VirtualType* get() const {
				 return virtual_type_ptr;
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
