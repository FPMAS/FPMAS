#ifndef FPMAS_MODEL_GUARDS
#define FPMAS_MODEL_GUARDS

/** \file src/fpmas/model/guards.h
 * Model Guards implementation.
 */

#include "fpmas/api/model/model.h"
#include "fpmas/synchro/guards.h"

namespace fpmas {
	namespace model {

		class ReadGuard : synchro::ReadGuard<fpmas::api::model::AgentPtr> {
			public:
				ReadGuard(api::model::Agent* agent)
					: synchro::ReadGuard<fpmas::api::model::AgentPtr>(agent->node()) {}
		};

		class AcquireGuard : synchro::AcquireGuard<fpmas::api::model::AgentPtr> {
			public:
				AcquireGuard(api::model::Agent* agent)
					: synchro::AcquireGuard<fpmas::api::model::AgentPtr>(agent->node()) {}
		};
	}
}
#endif
