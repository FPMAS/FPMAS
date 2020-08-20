#ifndef SCHEDULER_H
#define SCHEDULER_H

/** \file src/fpmas/scheduler/scheduler.h
 * Scheduler implementation.
 */

#include <unordered_map>

#include "fpmas/api/scheduler/scheduler.h"

namespace fpmas { namespace scheduler {
	using api::scheduler::Date;
	using api::scheduler::Period;
	using api::scheduler::JID;

	/**
	 * A Task that does not perform any operation.
	 *
	 * Might be used as a default Task.
	 */
	class VoidTask : public api::scheduler::Task {
		public:
			/**
			 * Immediatly returns.
			 */
			void run() override {};
	};

	/**
	 * api::scheduler::Job implementation.
	 */
	class Job : public api::scheduler::Job {
		private:
			VoidTask voidTask;
			JID _id;
			std::vector<api::scheduler::Task*> _tasks;
			api::scheduler::Task* _begin = &voidTask;
			api::scheduler::Task* _end = &voidTask;

		public:
			/**
			 * Job constructor.
			 *
			 * @param id job id
			 */
			Job(JID id) : _id(id) {}
			JID id() const override;
			void add(api::scheduler::Task&) override;
			void remove(api::scheduler::Task&) override;
			const std::vector<api::scheduler::Task*>& tasks() const override;
			TaskIterator begin() const override;
			TaskIterator end() const override;

			void setBeginTask(api::scheduler::Task&) override;
			void setEndTask(api::scheduler::Task&) override;

			api::scheduler::Task& getBeginTask() const override;
			api::scheduler::Task& getEndTask() const override;
	};

	/**
	 * api::scheduler::Epoch implementation.
	 */
	class Epoch : public api::scheduler::Epoch {
		private:
			std::vector<const api::scheduler::Job*> _jobs;
		public:
			void submit(const api::scheduler::Job&) override;
			const std::vector<const api::scheduler::Job*>& jobs() const override;
			JobIterator begin() const override;
			JobIterator end() const override;
			size_t jobCount() override;

			void clear() override;
	};



	/**
	 * api::scheduler::Scheduler implementation.
	 */
	class Scheduler : public api::scheduler::Scheduler {
		private:
			std::unordered_map<Date, std::vector<const api::scheduler::Job*>> unique_jobs;
			std::map<Date, std::vector<std::pair<Period, const api::scheduler::Job*>>>
				recurring_jobs;
			std::map<Date, std::vector<std::tuple<Date, Period, const api::scheduler::Job*>>>
				limited_recurring_jobs;
			void resizeCycle(size_t new_size);

		public:
			void schedule(api::scheduler::Date date, const api::scheduler::Job&) override;
			void schedule(api::scheduler::Date date, api::scheduler::Period period, const api::scheduler::Job&) override;
			void schedule(api::scheduler::Date date, api::scheduler::Date end, api::scheduler::Period period, const api::scheduler::Job&) override;
			void build(api::scheduler::Date date, fpmas::api::scheduler::Epoch&) const override;
	};

}}
#endif
