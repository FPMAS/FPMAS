#include "test_agents.h"

const fpmas::model::Behavior<TestSpatialAgent> TestSpatialAgent::behavior {
	&TestSpatialAgent::moveToNextCell};

fpmas::model::VonNeumannRange<fpmas::model::VonNeumannGrid<>> TestGridAgent::range(1);
