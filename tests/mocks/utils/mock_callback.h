#ifndef MOCK_CALLBACK_H
#define MOCK_CALLBACK_H
#include "fpmas/api/utils/callback.h"
#include "gmock/gmock.h"

template<typename... Args>
class MockCallback : public fpmas::api::utils::Callback<Args...> {
	public:
		MOCK_METHOD(void, call, (Args...), (override));
};

template<typename Event>
class MockEventCallback : public fpmas::api::utils::EventCallback<Event> {
	public:
		MOCK_METHOD(void, call, (const Event&), (override));
};
#endif
