#ifndef FPMAS_MOCK_BREAKPOINT_H
#define FPMAS_MOCK_BREAKPOINT_H

#include "gmock/gmock.h"
#include "fpmas/api/io/breakpoint.h"

template<typename T>
class MockBreakpoint : public fpmas::api::io::Breakpoint<T> {
	public:
		MOCK_METHOD(void, dump, (std::ostream&, const T&), (override));
		MOCK_METHOD(void, load, (std::istream&, T&), (override));
};
#endif
