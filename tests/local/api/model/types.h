#ifndef FPMAS_TEST_TYPES_H
#define FPMAS_TEST_TYPES_H

#include "../mocks/model/mock_model.h"
#include "utils/macros.h"

FPMAS_JSON_SERIALIZE_AGENT(MockAgent<4>, MockAgent<2>, MockAgent<12>)

#endif
