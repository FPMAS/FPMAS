#include "api/graph/parallel/location_manager.h"

#include "gmock/gmock.h"
#include "api/communication/communication.h"

template<typename DistNode>
class MockLocationManager : public FPMAS::api::graph::parallel::LocationManager<DistNode> {
	public:
		using typename FPMAS::api::graph::parallel::LocationManager<DistNode>::node_map;

		MockLocationManager(FPMAS::api::communication::MpiCommunicator&) {}

		MOCK_METHOD(
			void, updateLocations,
			(node_map, node_map),
			(override)
			);

		MOCK_METHOD(void, addManagedNode, (DistNode*, int), (override));
		MOCK_METHOD(void, removeManagedNode, (DistNode*), (override));
};
