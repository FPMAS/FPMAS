#include "model.h"

namespace FPMAS::api::model {

	AgentPtrWrapper::AgentPtrWrapper(AgentPtrWrapper&& other) {
		this->virtual_type_ptr = other.get();
		other.virtual_type_ptr = nullptr;
	}

	AgentPtrWrapper& AgentPtrWrapper::operator=(AgentPtrWrapper&& other) {
		AgentTask* task = nullptr;
		api::model::AgentNode* node = nullptr;
		if(this->virtual_type_ptr != nullptr) {
			task = this->virtual_type_ptr->task();
			node = this->virtual_type_ptr->node();
			delete this->virtual_type_ptr;
		}

		this->virtual_type_ptr = other.get();
		other.virtual_type_ptr = nullptr;
		this->virtual_type_ptr->setTask(task);
		this->virtual_type_ptr->setNode(node);
		task->setAgent(this->virtual_type_ptr);

		return *this;
	}

	AgentPtrWrapper::~AgentPtrWrapper() {
		if(this->virtual_type_ptr!=nullptr)
			delete this->virtual_type_ptr;
	}
}
