#include "fpmas/api/graph/location_manager.h"

#include "gmock/gmock.h"
#include "fpmas/api/communication/communication.h"

template<typename T>
class MockLocationManager : public fpmas::api::graph::LocationManager<T> {
	public:
		using typename fpmas::api::graph::LocationManager<T>::DistNode;
		using typename fpmas::api::graph::LocationManager<T>::NodeMap;

		MockLocationManager(
				fpmas::api::communication::MpiCommunicator&,
				fpmas::api::communication::TypedMpi<DistributedId>&,
				fpmas::api::communication::TypedMpi<std::pair<DistributedId, int>>&
				) {}

		MOCK_METHOD(void, updateLocations, (const NodeMap&), (override));

		MOCK_METHOD(void, addManagedNode, (DistNode*, int), (override));
		MOCK_METHOD(void, removeManagedNode, (DistNode*), (override));
		
		MOCK_METHOD(const NodeMap&, getLocalNodes, (), (const, override));
		MOCK_METHOD(const NodeMap&, getDistantNodes, (), (const, override));

		MOCK_METHOD(void, setLocal, (DistNode*), (override));
		MOCK_METHOD(void, setDistant, (DistNode*), (override));
		MOCK_METHOD(void, remove, (DistNode*), (override));
};
