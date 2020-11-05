#include "test_agents.h"

const fpmas::model::Behavior<TestLocatedAgent> TestLocatedAgent::behavior {
	&TestLocatedAgent::moveToNextCell};
