#include "scheduler.h"

namespace FPMAS::scheduler {

	JID Job::id() const {return _id;}

	void Job::add(api::scheduler::Task& task) {
		_tasks.push_back(&task);
	}

	const std::vector<api::scheduler::Task*>& Job::tasks() const {
		return _tasks;
	}

	void Job::setBeginTask(api::scheduler::Task& task) {
		this->_begin = &task;
	}

	api::scheduler::Task& Job::getBeginTask() const {
		return *_begin;
	}

	void Job::setEndTask(api::scheduler::Task& task) {
		this->_end = &task;
	}

	api::scheduler::Task& Job::getEndTask() const {
		return *_end;
	}

	typename Job::TaskIterator Job::begin() const {
		return _tasks.begin();
	}

	typename Job::TaskIterator Job::end() const {
		return _tasks.end();
	}

	void Epoch::submit(const api::scheduler::Job& job) {
		_jobs.push_back(&job);
	}

	const std::vector<const api::scheduler::Job*>& Epoch::jobs() const {
		return _jobs;
	}

	typename Epoch::JobIterator Epoch::begin() const {
		return _jobs.begin();
	}

	typename Epoch::JobIterator Epoch::end() const {
		return _jobs.end();
	}

	size_t Epoch::jobCount() {
		return _jobs.size();
	}

	void Epoch::clear() {
		_jobs.clear();
	}

	void Scheduler::schedule(Date date, const api::scheduler::Job& job) {
		unique_jobs[date].push_back(&job);
	}

	void Scheduler::schedule(Date start, Period period, const api::scheduler::Job& job) {
		recurring_jobs[start].push_back({period, &job});
	}

	void Scheduler::schedule(Date start, Date end, Period period, const api::scheduler::Job& job) {
		limited_recurring_jobs[start].push_back({end, period, &job});
	}

	void Scheduler::build(Date date, FPMAS::api::scheduler::Epoch& epoch) const {
		epoch.clear();
		auto unique = unique_jobs.find(date);
		if(unique != unique_jobs.end()) {
			for(auto job : unique->second) {
				epoch.submit(*job);
			}
		}
		if(recurring_jobs.size() > 0) {
			auto bound = recurring_jobs.lower_bound(date);
			if(bound!=recurring_jobs.end() && bound->first == date) {
				bound++;
			}
			
			for(auto recurring = recurring_jobs.begin(); recurring != bound; recurring++) {
				Date start = recurring->first;
				for(auto job : recurring->second) {
					if((date-start) % job.first == 0) {
						epoch.submit(*job.second);
					}
				}
			}
		}
		if(limited_recurring_jobs.size() > 0) {
			auto bound = limited_recurring_jobs.lower_bound(date);
			if(bound!=limited_recurring_jobs.end() && bound->first == date) {
				bound++;
			}
			
			for(auto recurring = limited_recurring_jobs.begin(); recurring != bound; recurring++) {
				Date start = recurring->first;
				for(auto job : recurring->second) {
					Date end = std::get<0>(job);
					Period period = std::get<1>(job);
					auto job_ptr = std::get<2>(job);
					if(date < end) {
						if((date-start) % period == 0) {
							epoch.submit(*job_ptr);
						}
					}
				}
			}
		}
	}
}
