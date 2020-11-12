#include "test_agents.h"

const fpmas::model::Behavior<TestSpatialAgent> TestSpatialAgent::behavior {
	&TestSpatialAgent::moveToNextCell};
