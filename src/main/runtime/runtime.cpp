#include "runtime.h"

namespace FPMAS::runtime {

	void Runtime::execute(Date start, Date end) {
		for(Date time = start; time < end; time++) {
			scheduler.build(time, epoch);
			for(api::scheduler::Job* job : epoch) {
				job->getBeginTask()->execute();
				for(api::scheduler::Task* task : *job) {
					task->execute();
				}
				job->getEndTask()->execute();
			}
		}
	}

	void Runtime::execute(Date end) {
		execute(0, end);
	}
}
