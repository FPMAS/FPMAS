#include "test_agents.h"

FPMAS_JSON_SET_UP(TEST_AGENTS)

const fpmas::model::Behavior<TestSpatialAgent> TestSpatialAgent::behavior {
	&TestSpatialAgent::moveToNextCell};
