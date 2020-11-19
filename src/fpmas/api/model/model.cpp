#include "model.h"

namespace fpmas { namespace api { namespace model {

	AgentPtr::AgentPtr(const AgentPtr& other)
		: api::utils::PtrWrapper<api::model::Agent>(other.ptr->copy()) {
	}

	AgentPtr::AgentPtr(AgentPtr&& other)
		: api::utils::PtrWrapper<api::model::Agent>(other.get()) {
		other.ptr = nullptr;
	}

	AgentPtr& AgentPtr::operator=(const AgentPtr& other) {
		if(this->ptr != nullptr)
			this->ptr->copyAssign(other.ptr);
		else
			if(other.ptr != nullptr)
				this->ptr = other.ptr->copy();
		return *this;
	}

	AgentPtr& AgentPtr::operator=(AgentPtr&& other) {
		if(this->ptr != nullptr) {
			if(other.ptr != nullptr) {
				this->ptr->moveAssign(other.get());
			}
		} else {
			this->ptr = other.get();
			other.ptr = nullptr;
		}

		return *this;
	}

	AgentPtr::~AgentPtr() {
		if(this->ptr!=nullptr)
			delete this->ptr;
	}
}}}
