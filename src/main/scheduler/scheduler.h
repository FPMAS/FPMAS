#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <unordered_map>
#include <map>

#include "api/scheduler/scheduler.h"

namespace FPMAS::scheduler {

	class VoidTask : public api::scheduler::Task {
		public:
			void run() override {};
	};

	class Job : public api::scheduler::Job {
		private:
			VoidTask voidTask;
			JID _id;
			std::vector<api::scheduler::Task*> _tasks;
			api::scheduler::Task* _begin = &voidTask;
			api::scheduler::Task* _end = &voidTask;

		public:
			Job(JID id) : _id(id) {}
			JID id() const override;
			void add(api::scheduler::Task*) override;
			const std::vector<api::scheduler::Task*>& tasks() const override;
			TaskIterator begin() override;
			TaskIterator end() override;

			void setBeginTask(api::scheduler::Task*) override;
			void setEndTask(api::scheduler::Task*) override;

			api::scheduler::Task* getBeginTask() const override;
			api::scheduler::Task* getEndTask() const override;
	};

	class Epoch : public api::scheduler::Epoch {
		private:
			std::vector<api::scheduler::Job*> _jobs;
		public:
			void submit(api::scheduler::Job*) override;
			const std::vector<api::scheduler::Job*>& jobs() const override;
			JobIterator begin() override;
			JobIterator end() override;
			size_t jobCount() override;

			void clear() override;
	};



	class Scheduler : public api::scheduler::Scheduler {
		private:
			std::unordered_map<Date, std::vector<FPMAS::api::scheduler::Job*>> unique_jobs;
			std::map<Date, std::vector<std::pair<Period, FPMAS::api::scheduler::Job*>>>
				recurring_jobs;
			std::map<Date, std::vector<std::tuple<Date, Period, FPMAS::api::scheduler::Job*>>>
				limited_recurring_jobs;
			void resizeCycle(size_t new_size);

		public:
			void schedule(Date date, FPMAS::api::scheduler::Job*) override;
			void schedule(Date date, Period period, FPMAS::api::scheduler::Job*) override;
			void schedule(Date date, Date end, Period period, FPMAS::api::scheduler::Job*) override;

			void build(Date date, FPMAS::api::scheduler::Epoch&) const override;
	};

}
#endif
