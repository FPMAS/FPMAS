#ifndef FPMAS_SERIALIZATION_TEST_AGENT_H
#define FPMAS_SERIALIZATION_TEST_AGENT_H

#include "fpmas/model/model.h"

template<typename AgentType>
class CustomAgentBase : public fpmas::model::AgentBase<AgentType> {
	protected:
		float data;

	public:
		CustomAgentBase(float data) : data(data) {
		}

		float getData() const {
			return data;
		}
};

struct CustomAgent : public CustomAgentBase<CustomAgent> {
	using CustomAgentBase::CustomAgentBase;
};

class DefaultConstructibleCustomAgent :
	public CustomAgentBase<DefaultConstructibleCustomAgent> {
	public:
		using CustomAgentBase::CustomAgentBase;

		DefaultConstructibleCustomAgent() : CustomAgentBase(7.4f) {
		}
};

struct CustomAgentWithLightPack : public CustomAgentBase<CustomAgentWithLightPack> {
	int very_important_data;

	CustomAgentWithLightPack(int very_important_data) :
		CustomAgentBase(4.2f), very_important_data(very_important_data) {
		}

	CustomAgentWithLightPack(float data, int very_important_data) :
		CustomAgentBase(data), very_important_data(very_important_data) {
		}
};
#endif
