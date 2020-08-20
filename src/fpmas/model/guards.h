#ifndef FPMAS_MODEL_GUARDS
#define FPMAS_MODEL_GUARDS

/** \file src/fpmas/model/guards.h
 * Model Guards implementation.
 */

#include "fpmas/api/model/model.h"
#include "fpmas/synchro/guards.h"

namespace fpmas {
	namespace model {

		/**
		 * \Agent read Guard.
		 *
		 * @see synchro::ReadGuard
		 */
		class ReadGuard : public synchro::ReadGuard<fpmas::api::model::AgentPtr> {
			public:
				/**
				 * ReadGuard constructor.
				 *
				 * @param agent agent to read
				 */
				ReadGuard(api::model::Agent* agent)
					: synchro::ReadGuard<fpmas::api::model::AgentPtr>(agent->node()) {}
		};

		/**
		 * \Agent acquire Guard.
		 *
		 * @see synchro::AcquireGuard
		 */
		class AcquireGuard : public synchro::AcquireGuard<fpmas::api::model::AgentPtr> {
			public:
				/**
				 * AcquireGuard constructor.
				 *
				 * @param agent agent to acquire
				 */
				AcquireGuard(api::model::Agent* agent)
					: synchro::AcquireGuard<fpmas::api::model::AgentPtr>(agent->node()) {}
		};
	}
}
#endif
