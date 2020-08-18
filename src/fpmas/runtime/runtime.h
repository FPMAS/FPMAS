#ifndef FPMAS_RUNTIME_H
#define FPMAS_RUNTIME_H

/** \file src/fpmas/runtime/runtime.h
 * Runtime implementation.
 */

#include "fpmas/api/runtime/runtime.h"
#include "fpmas/scheduler/scheduler.h"


namespace fpmas { namespace runtime {
	using api::runtime::Date;

	/**
	 * api::runtime::Runtime implementation.
	 */
	class Runtime : public api::runtime::Runtime {
		private:
			api::scheduler::Scheduler& scheduler;
			scheduler::Epoch epoch;
			Date date = 0;

		public:
			/**
			 * Runtime constructor.
			 *
			 * @param scheduler scheduler to execute
			 */
			Runtime(api::scheduler::Scheduler& scheduler)
				: scheduler(scheduler) {}

			void run(Date end) override;
			void run(Date start, Date end) override;
			Date currentDate() const override {return date;}
	};
}}
#endif
