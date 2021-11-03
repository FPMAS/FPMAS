#ifndef FPMAS_MODEL_DATAPACK_SERIALIZER_H
#define FPMAS_MODEL_DATAPACK_SERIALIZER_H

#include "serializer_set_up.h"
#include "fpmas/api/model/model.h"
#include "fpmas/io/datapack.h"

namespace fpmas { namespace io { namespace datapack {
	using api::model::AgentPtr;
	using api::model::WeakAgentPtr;

	template<>
		struct Serializer<AgentPtr> : public JsonSerializer<AgentPtr> {
		};
	template<>
		struct LightSerializer<AgentPtr> : public LightJsonSerializer<AgentPtr> {
		};
}}}

#endif
