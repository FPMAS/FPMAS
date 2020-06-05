#include "api/graph/parallel/location_manager.h"

#include "gmock/gmock.h"
#include "api/communication/communication.h"

template<typename T>
class MockLocationManager : public FPMAS::api::graph::parallel::LocationManager<T> {
	public:
		using typename FPMAS::api::graph::parallel::LocationManager<T>::DistNode;
		using typename FPMAS::api::graph::parallel::LocationManager<T>::NodeMap;

		MockLocationManager(
				FPMAS::api::communication::MpiCommunicator&,
				FPMAS::api::communication::TypedMpi<DistributedId>&,
				FPMAS::api::communication::TypedMpi<std::pair<DistributedId, int>>&
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
