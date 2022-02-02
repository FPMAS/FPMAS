#ifndef SCHEDULER_H
#define SCHEDULER_H

/** \file src/fpmas/scheduler/scheduler.h
 * Scheduler implementation.
 */

#include <unordered_map>
#include <queue>

#include "fpmas/api/scheduler/scheduler.h"

namespace fpmas { namespace scheduler {
	using api::scheduler::Date;
	using api::scheduler::Period;
	using api::scheduler::TimeStep;
	using api::scheduler::SubTimeStep;
	using api::scheduler::JID;
	using api::scheduler::JobList;
	using api::scheduler::time_step;
	using api::scheduler::sub_time_step;
	using api::scheduler::sub_step_end;

	/**
	 * Base task implementation.
	 *
	 * Only the setJobPos() and getJobPos() methods are implemented.
	 *
	 * @tparam TaskInterface implemented task interface (e.g.
	 * fpmas::api::scheduler::Task or fpmas::api::scheduler::NodeTask)
	 */
	template<typename TaskInterface>
		class TaskBase : public TaskInterface {
			private:
				std::unordered_map<
					JID,
					std::list<fpmas::api::scheduler::Task*>::iterator
						> job_pos;

			public:
				/**
				 * \copydoc fpmas::api::scheduler::Task::setJobPos()
				 */
				void setJobPos(
						JID job_id,
						std::list<fpmas::api::scheduler::Task*>::iterator pos
						) override {
					job_pos[job_id] = pos;
				}

				/**
				 * \copydoc fpmas::api::scheduler::Task::getJobPos()
				 */
				std::list<api::scheduler::Task*>::iterator getJobPos(JID job_id) const override {
					return job_pos.find(job_id)->second;
				}
		};

	/**
	 * A Task that does not perform any operation.
	 *
	 * Might be used as a default Task.
	 */
	class VoidTask : public TaskBase<api::scheduler::Task> {
		public:
			/**
			 * Immediatly returns.
			 */
			void run() override {};
	};

	namespace detail {
		// TODO 2.0: put this in fpmas::scheduler, rename
		// fpmas::scheduler::LambdaTask
		/**
		 * api::scheduler::Task implementation based on a lambda function.
		 */
		class LambdaTask : public TaskBase<api::scheduler::Task> {
			private:
				std::function<void()> fct;

			public:
				/**
				 * LambdaTask constructor.
				 *
				 * The specified lambda function must have a `void ()` signature.
				 *
				 * @tparam automatically deduced lambda function type
				 * @param lambda_fct void() lambda function, that will be run by
				 * the task 
				 */
				template<typename Lambda_t>
					LambdaTask(Lambda_t&& lambda_fct)
					: fct(lambda_fct) {}

				void run() override {
					fct();
				}
		};
	}

	/**
	 * A NodeTask that can be initialized from a lambda function.
	 *
	 * Used by the FPMAS_NODE_TASK() macro.
	 */
	template<typename T>
	class LambdaTask : public TaskBase<api::scheduler::NodeTask<T>> {
		private:
			std::function<void()> fct;
			api::graph::DistributedNode<T>* _node;

		public:
			/**
			 * LambdaTask constructor.
			 *
			 * The specified lambda function must have a `void ()` signature.
			 *
			 * @tparam automatically deduced lambda function type
			 * @param node node bound to the task
			 * @param lambda_fct void() lambda function, that will be run by
			 * the task 
			 */
			template<typename Lambda_t>
				LambdaTask(
						api::graph::DistributedNode<T>* node,
						Lambda_t&& lambda_fct)
				: _node(node), fct(lambda_fct) {}

			api::graph::DistributedNode<T>* node() override {
				return _node;
			}

			void run() override {
				fct();
			}
	};

	/**
	 * api::scheduler::Job implementation.
	 */
	class Job : public api::scheduler::Job {
		private:
			static JID job_id;
			VoidTask voidTask;
			JID _id;
			std::list<api::scheduler::Task*> _tasks;
			api::scheduler::Task* _begin = &voidTask;
			api::scheduler::Task* _end = &voidTask;

		public:
			/**
			 * Job constructor.
			 *
			 * @param id job id
			 */
			Job() : _id(job_id++) {}
			/**
			 * Initializes a Job from the specified tasks.
			 *
			 * @param tasks initial tasks
			 */
			Job(std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks);
			/**
			 * Initializes a Job from the specified tasks.
			 *
			 * @param begin begin task
			 * @param tasks initial tasks
			 */
			Job(
					api::scheduler::Task& begin,
					std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks);
			/**
			 * Initializes a Job from the specified tasks.
			 *
			 * @param begin begin task
			 * @param tasks initial tasks
			 * @param end end task
			 */
			Job(
					api::scheduler::Task& begin,
					std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks,
					api::scheduler::Task& end);
			/**
			 * Initializes a Job from the specified tasks.
			 *
			 * @param tasks initial tasks
			 * @param end end task
			 */
			Job(
					std::initializer_list<std::reference_wrapper<api::scheduler::Task>> tasks,
					api::scheduler::Task& end);

			JID id() const override;
			void add(api::scheduler::Task&) override;
			void remove(api::scheduler::Task&) override;
			std::vector<api::scheduler::Task*> tasks() const override;
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
			std::vector<SubTimeStep> job_ordering;

		public:
			void submit(const api::scheduler::Job&, api::scheduler::SubTimeStep sub_time_step) override;
			void submit(api::scheduler::JobList job_list, api::scheduler::SubTimeStep sub_time_step) override;
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
			struct SchedulerItem {
				SubTimeStep sub_step;
				JobList job_list;

				SchedulerItem(SubTimeStep sub_step, JobList job_list)
					: sub_step(sub_step), job_list(job_list) {}
			};

			std::unordered_map<TimeStep, std::vector<SchedulerItem>> unique_jobs;
			std::map<TimeStep, std::vector<std::pair<Period, SchedulerItem>>>
				recurring_jobs;
			std::map<TimeStep, std::vector<std::tuple<Date, Period, SchedulerItem>>>
				limited_recurring_jobs;
			void resizeCycle(size_t new_size);

		public:
			void schedule(api::scheduler::Date date, const api::scheduler::Job&) override;
			void schedule(api::scheduler::Date date, api::scheduler::Period period, const api::scheduler::Job&) override;
			void schedule(api::scheduler::Date date, api::scheduler::Date end, api::scheduler::Period period, const api::scheduler::Job&) override;
			void schedule(api::scheduler::Date date, api::scheduler::JobList) override;
			void schedule(api::scheduler::Date date, api::scheduler::Period period, api::scheduler::JobList) override;
			void schedule(api::scheduler::Date date, api::scheduler::Date end, api::scheduler::Period period, api::scheduler::JobList) override;
			void build(api::scheduler::TimeStep step, fpmas::api::scheduler::Epoch&) const override;
	};

}}
#endif
