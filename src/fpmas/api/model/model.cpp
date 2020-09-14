#include "model.h"

namespace fpmas { namespace api { namespace model {

	AgentPtr::AgentPtr(const AgentPtr& other) {
		this->ptr = other.ptr->copy();
		this->ptr->setGroupId(other.ptr->groupId());
	}

	AgentPtr::AgentPtr(AgentPtr&& other) {
		this->ptr = other.get();
		other.ptr = nullptr;
	}

	AgentPtr& AgentPtr::operator=(const AgentPtr& other) {
		this->ptr->copyAssign(other.ptr);
		this->ptr->setGroupId(other.ptr->groupId());
		return *this;
	}

	AgentPtr& AgentPtr::operator=(AgentPtr&& other) {
		if(other.ptr != nullptr) {
			this->ptr->moveAssign(other.get());
		}

		return *this;
	}

	AgentPtr::~AgentPtr() {
		if(this->ptr!=nullptr)
			delete this->ptr;
	}
}}}
