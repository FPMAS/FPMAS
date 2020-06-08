#ifndef RUNTIME_H
#define RUNTIME_H
#include "api/runtime/runtime.h"
#include "scheduler/scheduler.h"


namespace FPMAS::runtime {

	class Runtime : public api::runtime::Runtime {
		private:
			api::scheduler::Scheduler& scheduler;
			scheduler::Epoch epoch;

		public:
			Runtime(api::scheduler::Scheduler& scheduler)
				: scheduler(scheduler) {}

			void execute(Date end) override;
			void execute(Date start, Date end) override;
	};
}
#endif
