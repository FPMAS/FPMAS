#include "runtime.h"
#include <random>

namespace fpmas::runtime {

	void Runtime::run(Date start, Date end) {
		std::mt19937 rd;
		for(Date time = start; time < end; time++) {
			date = time;
			scheduler.build(time, epoch);
			for(const api::scheduler::Job* job : epoch) {
				job->getBeginTask().run();
				std::vector<api::scheduler::Task*> tasks = job->tasks();
				std::shuffle(tasks.begin(), tasks.end(), rd);
				for(api::scheduler::Task* task : tasks) {
					task->run();
				}
				job->getEndTask().run();
			}
		}
	}

	void Runtime::run(Date end) {
		run(0, end);
	}
}
