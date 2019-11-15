#include "agent_base.h"

using FPMAS::agent::AgentBase;

AgentBase::AgentBase(int weight) : weight(weight) {};

int AgentBase::getWeight() const {
	return this->weight;
}

namespace FPMAS {
	namespace Agent {
		void to_json(json& j, const AgentBase& agent) {
			j = {{ "weight", agent.getWeight() }};
			agent.serialize(j);
		}
	}
}
