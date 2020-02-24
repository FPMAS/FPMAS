#include "agent.h"

namespace FPMAS::agent {

	AgentPtr::AgentPtr(Agent* agent) : agent(agent) {}

	AgentPtr::AgentPtr(AgentPtr&& agent_ptr) : agent(agent_ptr.agent) {
		agent_ptr.agent = nullptr;
	}

	AgentPtr& AgentPtr::operator=(AgentPtr&& agent_ptr) {
		delete this->agent;
		this->agent = agent_ptr.agent;
		agent_ptr.agent = nullptr;

		return *this;
	}

	AgentPtr::~AgentPtr() {
		delete this->agent;
	}
}
