#include "scheduler.h"
#include <algorithm>

namespace fpmas { namespace scheduler {

	Job::Job(std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks)
		: Job() {
			for(api::scheduler::Task& task : tasks)
				add(task);
		}

	Job::Job(
			api::scheduler::Task& begin,
			std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks)
		: Job(tasks) {
			setBeginTask(begin);
		}

	Job::Job(
			api::scheduler::Task& begin,
			std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks,
			api::scheduler::Task& end)
		: Job(begin, tasks) {
			setEndTask(end);
		}

	Job::Job(
			std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks,
			api::scheduler::Task& end)
		: Job(tasks) {
			setEndTask(end);
		}

	JID Job::job_id {0};

	JID Job::id() const {return _id;}

	void Job::add(api::scheduler::Task& task) {
		_tasks.push_back(&task);
	}

	void Job::remove(api::scheduler::Task& task) {
		auto removed = std::remove(_tasks.begin(), _tasks.end(), &task);
		if(removed != _tasks.end())
			_tasks.erase(removed);
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

	void Epoch::submit(const api::scheduler::Job& job, api::scheduler::SubTimeStep sub_step) {
		this->submit(JobList {job}, sub_step);
	}
	void Epoch::submit(api::scheduler::JobList job_list, api::scheduler::SubTimeStep sub_step) {
		std::vector<const api::scheduler::Job*>::iterator job_pos = _jobs.begin();
		std::vector<SubTimeStep>::iterator job_ordering_pos = job_ordering.begin();

		while(job_ordering_pos != job_ordering.end() && sub_step >= *job_ordering_pos) {
			++job_pos;
			++job_ordering_pos;
		}

		std::vector<SubTimeStep> sub_time_steps;
		std::vector<const api::scheduler::Job*> jobs;
		for(const api::scheduler::Job& job : job_list) {
			sub_time_steps.push_back(sub_step);
			jobs.push_back(&job);
		}

		job_ordering.insert(job_ordering_pos, sub_time_steps.begin(), sub_time_steps.end());
		_jobs.insert(job_pos, jobs.begin(), jobs.end());
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
		job_ordering.clear();
		_jobs.clear();
	}

	void Scheduler::schedule(api::scheduler::Date date, const api::scheduler::Job& job) {
		this->schedule(date, JobList {job});
	}

	void Scheduler::schedule(api::scheduler::Date date, api::scheduler::JobList job_list) {
		float step_f;
		api::scheduler::SubTimeStep sub_step = std::modf(date, &step_f);
		api::scheduler::TimeStep step = step_f;

		unique_jobs[step].push_back({sub_step, {job_list}});
	}

	void Scheduler::schedule(
			api::scheduler::Date start,
			api::scheduler::Period period,
			const api::scheduler::Job& job
			) {
		this->schedule(start, period, JobList {job});
	}
	void Scheduler::schedule(
			api::scheduler::Date start,
			api::scheduler::Period period,
			api::scheduler::JobList job_list
			) {
		float step_f;
		api::scheduler::SubTimeStep sub_step = std::modf(start, &step_f);
		api::scheduler::TimeStep step = step_f;

		recurring_jobs[step].push_back({period, {sub_step, {job_list}}});
	}

	void Scheduler::schedule(
			api::scheduler::Date start,
			api::scheduler::Date end,
			api::scheduler::Period period,
			const api::scheduler::Job& job
			) {
		this->schedule(start, end, period, JobList {job});

	}
	void Scheduler::schedule(
			api::scheduler::Date start,
			api::scheduler::Date end,
			api::scheduler::Period period,
			api::scheduler::JobList job_list
			) {
		float step_f;
		api::scheduler::SubTimeStep sub_step = std::modf(start, &step_f);
		api::scheduler::TimeStep step = step_f;

		limited_recurring_jobs[step].push_back({end, period, {sub_step, {job_list}}});
	}

	void Scheduler::build(api::scheduler::TimeStep step, fpmas::api::scheduler::Epoch& epoch) const {
		epoch.clear();
		auto unique = unique_jobs.find(step);
		if(unique != unique_jobs.end()) {
			for(auto job_item : unique->second) {
				epoch.submit(job_item.job_list, job_item.sub_step);
			}
		}
		if(recurring_jobs.size() > 0) {
			auto bound = recurring_jobs.lower_bound(step);
			if(bound!=recurring_jobs.end() && bound->first == step) {
				bound++;
			}
			
			for(auto recurring = recurring_jobs.begin(); recurring != bound; recurring++) {
				TimeStep start = recurring->first;
				for(auto job_item : recurring->second) {
					if((step-start) % job_item.first == 0) {
						epoch.submit(job_item.second.job_list, job_item.second.sub_step);
					}
				}
			}
		}
		if(limited_recurring_jobs.size() > 0) {
			auto bound = limited_recurring_jobs.lower_bound(step);
			if(bound!=limited_recurring_jobs.end() && bound->first == step) {
				bound++;
			}
			
			for(auto recurring = limited_recurring_jobs.begin(); recurring != bound; recurring++) {
				TimeStep start = recurring->first;
				for(auto limited_job : recurring->second) {
					Date end = std::get<0>(limited_job);
					Period period = std::get<1>(limited_job);
					auto job_item = std::get<2>(limited_job);
					if(step + job_item.sub_step < end) {
						if((step-start) % period == 0) {
							epoch.submit(job_item.job_list, job_item.sub_step);
						}
					}
				}
			}
		}
	}
}}
