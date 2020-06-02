#include "scheduler.h"

namespace FPMAS::scheduler {

	JID Job::id() const {return _id;}

	void Job::add(api::scheduler::Task * task) {
		_tasks.push_back(task);
	}

	const std::vector<api::scheduler::Task*>& Job::tasks() const {
		return _tasks;
	}

	void Job::setBegin(api::scheduler::Task* task) {
		this->begin = task;
	}

	api::scheduler::Task* Job::getBegin() const {
		return begin;
	}

	void Job::setEnd(api::scheduler::Task* task) {
		this->end = task;
	}

	api::scheduler::Task* Job::getEnd() const {
		return end;
	}

	void Epoch::submit(api::scheduler::Job * job) {
		_jobs.push_back(job);
	}

	const std::vector<api::scheduler::Job*>& Epoch::jobs() const {
		return _jobs;
	}

	void Epoch::clear() {
		_jobs.clear();
	}

	Job& Scheduler::schedule(Date date, ScheduleOverlapPolicy policy) {
		Job* new_job = new Job(id++);
		jobs.push_back(new_job);
		auto unique_epoch = unique_epochs.find(date);
		switch(policy) {
			case OVERRIDE:
				if(unique_epoch == unique_epochs.end()) {
					unique_epochs[date].submit(new_job);
				} else {
					unique_epoch->second.clear();
					unique_epoch->second.submit(new_job);
				}
				break;
			case STACK:
				if(unique_epoch == unique_epochs.end()) {
					auto& new_epoch = unique_epochs[date];
					Epoch recurring_epoch = cycle[date % cycle.size()];
					for(auto* job : recurring_epoch.jobs()) {
						new_epoch.submit(job);
					}
					new_epoch.submit(new_job);
				} else {
					unique_epoch->second.submit(new_job);
				}
				break;
		}
		return *new_job;
	}

	void Scheduler::resizeCycle(size_t new_period) {
		size_t old_size = cycle.size();
		size_t count = 1;
		while(cycle.size() < new_period) {
			cycle.resize((count+1) * old_size);
			for(size_t j = count * old_size; j < cycle.size(); j++) {
				for(size_t i = 0; i < old_size; i++) {
					for(auto job : cycle[i].jobs()) {
						cycle[j].submit(job);
					}
				}
			}
			count++;
		}
	}

	Job& Scheduler::schedule(Date date, Period period, ScheduleOverlapPolicy policy) {
		if(period == 0) {
			return schedule(date, policy);
		}
		if(cycle.size() < period) {
			resizeCycle(period);
		}
		Job* new_job = new Job(id++);
		jobs.push_back(new_job);
		for(size_t e = 0; e < cycle.size(); e++) {
			if(e % period == 0) {
				switch(policy) {
					case OVERRIDE :
						cycle[e].clear();
						cycle[e].submit(new_job);
						break;
					case STACK :
						cycle[e].submit(new_job);
						break;
				}
			}
		}
		for(auto item : unique_epochs) {
			if(item.first % period == 0) {
				item.second.submit(new_job);
			}
		}
		return *new_job;
	}

	const Epoch& Scheduler::epoch(Date date) const {
		auto uniqueEpoch = unique_epochs.find(date);
		if(uniqueEpoch != unique_epochs.end()) {
			return uniqueEpoch->second;
		}
		else {
			return cycle[date % cycle.size()];
		}
	}
}
