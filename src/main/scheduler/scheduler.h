#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <unordered_map>

#include "api/scheduler/scheduler.h"

namespace FPMAS::scheduler {

	class VoidTask : public api::scheduler::Task {
		public:
			void execute() override {};
	};

	class Job : public api::scheduler::Job {
		private:
			VoidTask voidTask;
			JID _id;
			std::vector<api::scheduler::Task*> _tasks;
			api::scheduler::Task* begin = &voidTask;
			api::scheduler::Task* end = &voidTask;

		public:
			Job(JID id) : _id(id) {}
			JID id() const override;
			void add(api::scheduler::Task*) override;
			const std::vector<api::scheduler::Task*>& tasks() const override;

			void setBegin(api::scheduler::Task*) override;
			void setEnd(api::scheduler::Task*) override;

			api::scheduler::Task* getBegin() const override;
			api::scheduler::Task* getEnd() const override;
	};

	class Epoch : public api::scheduler::Epoch {
		private:
			std::vector<api::scheduler::Job*> _jobs;
		public:
			void submit(api::scheduler::Job*) override;
			const std::vector<api::scheduler::Job*>& jobs() const override;
			void clear() override;
	};

	class Scheduler : public api::scheduler::Scheduler {
		private:
			JID id = 0;
			std::vector<Job*> jobs;
			std::vector<Epoch> cycle {1};
			std::unordered_map<Date, Epoch> unique_epochs;
			void resizeCycle(size_t new_size);

		public:
			Job& schedule(Date date, ScheduleOverlapPolicy policy) override;
			Job& schedule(Date date, Period period, ScheduleOverlapPolicy policy) override;

			const Epoch& epoch(Date date) const override;

	};

}

#endif
