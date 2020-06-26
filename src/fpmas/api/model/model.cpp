#include "model.h"

namespace fpmas::api::model {

	AgentPtrWrapper::AgentPtrWrapper(const AgentPtrWrapper& other) {
		this->virtual_type_ptr = other.virtual_type_ptr->copy();
		//this->virtual_type_ptr->setNode(other.virtual_type_ptr->node());
		//this->virtual_type_ptr->setTask(other.virtual_type_ptr->task());
		this->virtual_type_ptr->setGroupId(other.virtual_type_ptr->groupId());
	}

	AgentPtrWrapper::AgentPtrWrapper(AgentPtrWrapper&& other) {
		this->virtual_type_ptr = other.get();
		other.virtual_type_ptr = nullptr;
	}

	AgentPtrWrapper& AgentPtrWrapper::operator=(const AgentPtrWrapper& other) {
		if(other.virtual_type_ptr != nullptr) {
			//AgentTask* task = nullptr;
			//api::model::AgentNode* node = nullptr;
			if(this->virtual_type_ptr != nullptr) {
				//task = this->virtual_type_ptr->task();
				//node = this->virtual_type_ptr->node();
				delete this->virtual_type_ptr;
			}

			this->virtual_type_ptr = other.virtual_type_ptr->copy();
			//this->virtual_type_ptr->setTask(task);
			//this->virtual_type_ptr->setNode(node);
			this->virtual_type_ptr->setGroupId(other.virtual_type_ptr->groupId());
			//task->setAgent(this->virtual_type_ptr);
		}
		return *this;
	}

	AgentPtrWrapper& AgentPtrWrapper::operator=(AgentPtrWrapper&& other) {
		if(other.virtual_type_ptr != nullptr) {
			AgentTask* task = nullptr;
			api::model::AgentNode* node = nullptr;
			api::model::AgentGraph* graph = nullptr;
			if(this->virtual_type_ptr != nullptr) {
				task = this->virtual_type_ptr->task();
				node = this->virtual_type_ptr->node();
				graph = this->virtual_type_ptr->graph();
				delete this->virtual_type_ptr;
			}

			this->virtual_type_ptr = other.get();
			other.virtual_type_ptr = nullptr;
			this->virtual_type_ptr->setTask(task);
			this->virtual_type_ptr->setNode(node);
			this->virtual_type_ptr->setGraph(graph);
			task->setAgent(this->virtual_type_ptr);
		}

		return *this;
	}

	AgentPtrWrapper::~AgentPtrWrapper() {
		if(this->virtual_type_ptr!=nullptr)
			delete this->virtual_type_ptr;
	}
}
