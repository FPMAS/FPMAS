#ifndef RUNTIME_H
#define RUNTIME_H
#include "api/runtime/runtime.h"
#include "scheduler/scheduler.h"


namespace FPMAS::runtime {

	class Runtime : public api::runtime::Runtime {
		private:
			api::scheduler::Scheduler& scheduler;
			scheduler::Epoch epoch;
			Date date = 0;

		public:
			Runtime(api::scheduler::Scheduler& scheduler)
				: scheduler(scheduler) {}

			void run(Date end) override;
			void run(Date start, Date end) override;
			Date currentDate() const override {return date;}
	};
}
#endif
