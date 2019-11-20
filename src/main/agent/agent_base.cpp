#include "agent_base.h"

using FPMAS::agent::AgentBase;

AgentBase::AgentBase() {};

namespace FPMAS {
	namespace Agent {
		void to_json(json& j, const AgentBase& agent) {
//			j = {{ "weight", agent.getWeight() }};
//			agent.serialize(j);
		}
	}
}
