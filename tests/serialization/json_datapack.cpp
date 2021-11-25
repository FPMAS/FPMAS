#include "fpmas.h"
#include "json.h"
#include "datapack.h"

FPMAS_BASE_DATAPACK_SET_UP(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);
FPMAS_BASE_JSON_SET_UP(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);

int main(int argc, char** argv) {
	FPMAS_REGISTER_AGENT_TYPES(
		MockAgent<4>, MockAgent<12>,
		CustomAgent, DefaultConstructibleCustomAgent, CustomAgentWithLightPack
		);

	std::cout << "Running AgentPtr json + datapack test suit (FPMAS " << FPMAS_VERSION << ")" << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
