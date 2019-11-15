#ifndef AGENT_BASE_H
#define AGENT_BASE_H

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace FPMAS {
	namespace agent {

		class AgentBase {
			private:
				int weight;

			public:
				AgentBase(int weight);
				int getWeight() const;
				virtual void serialize(json& j) const = 0;



		};
		void to_json(json&, const AgentBase&);
	}
}
#endif
