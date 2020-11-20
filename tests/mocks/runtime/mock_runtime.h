#ifndef FPMAS_MOCK_RUNTIME_H
#define FPMAS_MOCK_RUNTIME_H

#include "gmock/gmock.h"
#include "fpmas/api/runtime/runtime.h"

class MockRuntime : public fpmas::api::runtime::Runtime {

	public:
		MOCK_METHOD(void, run, (fpmas::api::scheduler::Date, fpmas::api::scheduler::Date), (override));
		MOCK_METHOD(void, run, (fpmas::api::scheduler::Date), (override));
		MOCK_METHOD(void, execute, (const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(void, execute, (const fpmas::api::scheduler::JobList&), (override));
		MOCK_METHOD(fpmas::api::scheduler::Date, currentDate, (), (const, override));
};

#endif
