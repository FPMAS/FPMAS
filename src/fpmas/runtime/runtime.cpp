#include "runtime.h"

namespace fpmas { namespace runtime {

	// Initializes DistributedGenerator with a default seed
	random::DistributedGenerator<> Runtime::distributed_rd;

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
		std::vector<api::scheduler::Task*> tasks = {job.begin(), job.end()};
		std::shuffle(tasks.begin(), tasks.end(), distributed_rd);
		for(api::scheduler::Task* task : tasks) {
			task->run();
		}
		job.getEndTask().run();
	}

	void Runtime::execute(const api::scheduler::JobList &job_list) {
		for(auto& job : job_list)
			execute(job);
	}

	void Runtime::seed(random::DistributedGenerator<>::result_type seed) {
		Runtime::distributed_rd.seed(seed);
	}
}}
