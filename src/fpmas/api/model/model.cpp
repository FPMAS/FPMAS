#include "model.h"

namespace fpmas { namespace api { namespace model {

	AgentPtr::AgentPtr(const AgentPtr& other) {
		this->virtual_type_ptr = other.virtual_type_ptr->copy();
		this->virtual_type_ptr->setGroupId(other.virtual_type_ptr->groupId());
	}

	AgentPtr::AgentPtr(AgentPtr&& other) {
		this->virtual_type_ptr = other.get();
		other.virtual_type_ptr = nullptr;
	}

	AgentPtr& AgentPtr::operator=(const AgentPtr& other) {
		if(this->virtual_type_ptr != nullptr) {
			delete this->virtual_type_ptr;
		}
		this->virtual_type_ptr = other.virtual_type_ptr->copy();
		this->virtual_type_ptr->setGroupId(other.virtual_type_ptr->groupId());
		return *this;
	}

	AgentPtr& AgentPtr::operator=(AgentPtr&& other) {
		if(other.virtual_type_ptr != nullptr) {
			api::model::AgentGroup* group = nullptr;
			AgentTask* task = nullptr;
			api::model::AgentNode* node = nullptr;
			api::model::AgentGraph* graph = nullptr;
			if(this->virtual_type_ptr != nullptr) {
				group = this->virtual_type_ptr->group();
				task = this->virtual_type_ptr->task();
				task->setAgent(other.get());
				node = this->virtual_type_ptr->node();
				graph = this->virtual_type_ptr->graph();

				delete this->virtual_type_ptr;
			}

			this->virtual_type_ptr = other.get();
			other.virtual_type_ptr = nullptr;
			this->virtual_type_ptr->setGroup(group);
			this->virtual_type_ptr->setTask(task);
			this->virtual_type_ptr->setNode(node);
			this->virtual_type_ptr->setGraph(graph);
		}

		return *this;
	}

	AgentPtr::~AgentPtr() {
		if(this->virtual_type_ptr!=nullptr)
			delete this->virtual_type_ptr;
	}
}}}
