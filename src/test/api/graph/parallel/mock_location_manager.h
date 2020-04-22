#include "api/graph/parallel/location_manager.h"

#include "gmock/gmock.h"
#include "api/communication/communication.h"

template<typename DistNode>
class MockLocationManager : public FPMAS::api::graph::parallel::LocationManager<DistNode> {
	public:
		using typename FPMAS::api::graph::parallel::LocationManager<DistNode>::node_map;

		MockLocationManager(FPMAS::api::communication::MpiCommunicator&) {}

		MOCK_METHOD(void, updateLocations, (const node_map&), (override));

		MOCK_METHOD(void, addManagedNode, (DistNode*, int), (override));
		MOCK_METHOD(void, removeManagedNode, (DistNode*), (override));
		
		MOCK_METHOD(const node_map&, getLocalNodes, (), (const, override));
		MOCK_METHOD(const node_map&, getDistantNodes, (), (const, override));

		MOCK_METHOD(void, setLocal, (DistNode*), (override));
		MOCK_METHOD(void, setDistant, (DistNode*), (override));
		MOCK_METHOD(void, remove, (DistNode*), (override));
};
