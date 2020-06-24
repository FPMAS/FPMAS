#ifndef MOCK_REQUEST_HANDLER_H
#define MOCK_REQUEST_HANDLER_H

#include "gmock/gmock.h"
#include "fpmas/api/communication/request_handler.h"

class MockRequestHandler : public fpmas::api::communication::RequestHandler {
	public:
		MOCK_METHOD(std::string, read, (DistributedId, int), (override));
		MOCK_METHOD(std::string, acquire, (DistributedId, int), (override));
		MOCK_METHOD(void, giveBack, (DistributedId, int), (override));

		MOCK_METHOD(void, respondToRead, (int, DistributedId), (override));
		MOCK_METHOD(void, respondToAcquire, (int, DistributedId), (override));

		MOCK_METHOD(void, terminate, (), (override));
};

#endif
