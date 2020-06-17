#include "ptr_wrapper.h"

namespace FPMAS::api::utils {

	AgentPtrWrapper::AgentPtrWrapper(AgentPtrWrapper&& other) {
		this->virtual_type_ptr = other.get();
		other.virtual_type_ptr = nullptr;
	}

	AgentPtrWrapper& AgentPtrWrapper::operator=(AgentPtrWrapper&& other) {
		if(this->virtual_type_ptr != nullptr) {
			auto* task = this->virtual_type_ptr->task();

		}

		return *this;
	}
}
