#ifndef AGENT_H
#define AGENT_H

namespace FPMAS::agent {

	// functions to handle perceptions might be implemented
	class Agent {
		public:
			virtual void act() = 0;
			virtual ~Agent() {};

	};

	class AgentPtr {
		Agent* agent;

		public:
			AgentPtr(Agent*);
			AgentPtr(const AgentPtr&) = delete;
			AgentPtr& operator=(const AgentPtr&) = delete;

			AgentPtr(AgentPtr&&);
			AgentPtr& operator=(AgentPtr&&);

			~AgentPtr();

	};

}

#endif
