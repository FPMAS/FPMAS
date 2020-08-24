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
		if(this->ptr != nullptr) {
			delete this->ptr;
		}
		this->ptr = other.ptr->copy();
		this->ptr->setGroupId(other.ptr->groupId());
		return *this;
	}

	AgentPtr& AgentPtr::operator=(AgentPtr&& other) {
		if(other.ptr != nullptr) {
			api::model::AgentGroup* group = nullptr;
			AgentTask* task = nullptr;
			api::model::AgentNode* node = nullptr;
			api::model::Model* model = nullptr;
			if(this->ptr != nullptr) {
				group = this->ptr->group();
				task = this->ptr->task();
				task->setAgent(other.get());
				node = this->ptr->node();
				model = this->ptr->model();

				delete this->ptr;
			}

			this->ptr = other.get();
			other.ptr = nullptr;
			this->ptr->setGroup(group);
			this->ptr->setTask(task);
			this->ptr->setNode(node);
			this->ptr->setModel(model);
		}

		return *this;
	}

	AgentPtr::~AgentPtr() {
		if(this->ptr!=nullptr)
			delete this->ptr;
	}
}}}
