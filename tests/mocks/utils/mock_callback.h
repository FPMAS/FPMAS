#ifndef MOCK_CALLBACK_H
#define MOCK_CALLBACK_H
#include "api/utils/callback.h"
#include "gmock/gmock.h"

template<typename... Args>
class MockCallback : public fpmas::api::utils::Callback<Args...> {
	public:
		MOCK_METHOD(void, call, (Args...), (override));
};
#endif
