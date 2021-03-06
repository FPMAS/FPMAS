#include "runtime.h"

namespace fpmas { namespace runtime {

	void Runtime::run(Date start, Date end) {
		for(Date time = start; time < end; time++) {
			date = time;
			scheduler.build(time, epoch);
			for(const api::scheduler::Job* job : epoch) {
				execute(*job);
			}
		}
	}

	void Runtime::run(Date end) {
		run(date, end);
	}

	void Runtime::execute(const api::scheduler::Job &job) {
		job.getBeginTask().run();
		std::vector<api::scheduler::Task*> tasks = job.tasks();
		std::shuffle(tasks.begin(), tasks.end(), rd);
		for(api::scheduler::Task* task : tasks) {
			task->run();
		}
		job.getEndTask().run();
	}

	void Runtime::execute(const api::scheduler::JobList &job_list) {
		for(auto& job : job_list)
			execute(job);
	}
}}
