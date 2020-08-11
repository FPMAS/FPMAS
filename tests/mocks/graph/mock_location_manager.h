#include "fpmas/api/graph/location_manager.h"

#include "gmock/gmock.h"
#include "fpmas/api/communication/communication.h"

template<typename T>
class MockLocationManager : public fpmas::api::graph::LocationManager<T> {
	public:
		typedef fpmas::api::graph::DistributedNode<T> DistributedNode;
		using typename fpmas::api::graph::LocationManager<T>::NodeMap;

		MockLocationManager(
				fpmas::api::communication::MpiCommunicator&,
				fpmas::api::communication::TypedMpi<DistributedId>&,
				fpmas::api::communication::TypedMpi<std::pair<DistributedId, int>>&
				) {}

		MOCK_METHOD(void, updateLocations, (), (override));

		MOCK_METHOD(void, addManagedNode, (DistributedNode*, int), (override));
		MOCK_METHOD(void, removeManagedNode, (DistributedNode*), (override));
		
		MOCK_METHOD(const NodeMap&, getLocalNodes, (), (const, override));
		MOCK_METHOD(const NodeMap&, getDistantNodes, (), (const, override));

		MOCK_METHOD(void, setLocal, (DistributedNode*), (override));
		MOCK_METHOD(void, setDistant, (DistributedNode*), (override));
		MOCK_METHOD(void, remove, (DistributedNode*), (override));
};
