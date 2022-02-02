#ifndef FPMAS_SCHEDULER_API_H
#define FPMAS_SCHEDULER_API_H

/** \file src/fpmas/api/scheduler/scheduler.h
 * Scheduler API
 */

#include <list>

#include "fpmas/api/graph/distributed_node.h"

namespace fpmas { namespace api { namespace scheduler {
	/**
	 * Type used to define a time period.
	 */
	typedef unsigned long Period;
	/**
	 * Type used to define a particular date.
	 *
	 * The integer part of a Date corresponds to the TimeStep used to build an
	 * Epoch, and the rational part is the SubTimeStep, that can be used to
	 * order events within a TimeStep.
	 */
	typedef float Date;

	/**
	 * Integral part of a Date.
	 */
	typedef unsigned long TimeStep;
	/**
	 * Decimal part of a Date.
	 */
	typedef float SubTimeStep;

	/**
	 * Job ID type.
	 */
	typedef size_t JID;

	/**
	 * Returns the integer time step represented by `date`.
	 *
	 * @param date date
	 * @return time step
	 */
	TimeStep time_step(Date date);
	/**
	 * Returns the decimal sub time step represented by `date`.
	 *
	 * @param date date
	 * @return sub stime step
	 */
	SubTimeStep sub_time_step(Date date);

	/**
	 * The maximum Date value in the provided `time_step`.
	 *
	 * This value can be used to schedule a job after all other in the current
	 * time step.
	 *
	 * @par example
	 * ```cpp
	 * // Ordered list of jobs
	 * scheduler.schedule(0, 1, job1);
	 * scheduler.schedule(0.1, 1, job2);
	 * ...
	 * // Always executed at the end of each time step
	 * scheduler.schedule(fpmas::scheduler::sub_step_end(0), 1, final_job);
	 * ```
	 */
	api::scheduler::SubTimeStep sub_step_end(api::scheduler::TimeStep time_step);

	/**
	 * Task API.
	 */
	class Task {
		public:
			/**
			 * Runs the task.
			 */
			virtual void run() = 0;

			/**
			 * Sets a list position associated to the Job represented by
			 * `job_id`, that can be retrieved with getJobPos().
			 *
			 * There is no specific requirement about those methods, that can
			 * be used or not. However, Job implementations can take advantage
			 * of this feature to optimize Task insertion and removal in
			 * constant time, using an std::list as an internal data structure.
			 *
			 * @param job_id Job ID
			 * @param pos list iterator that references the current Task within
			 * the job associated to `job_id`
			 */
			virtual void setJobPos(JID job_id, std::list<Task*>::iterator pos) = 0;

			/**
			 * Retrieves a list position that was previously stored using
			 * setJobPos().
			 *
			 * @param job_id Job ID
			 * @return list iterator that references the current Task within
			 * the job associated to `job_id`
			 */
			virtual std::list<Task*>::iterator getJobPos(JID job_id) const = 0;

			virtual ~Task() {}
	};

	/**
	 * NodeTask API.
	 *
	 * A NodeTask is a Task associated to an api::graph::DistributedNode.
	 */
	template<typename T>
		class NodeTask : public Task {
			public:
				/**
				 * Returns a pointer to the node to which this task is
				 * associated.
				 *
				 * @return pointer to associated node
				 */
				virtual api::graph::DistributedNode<T>* node() = 0;
		};

	/**
	 * Job API.
	 *
	 * A Job is a set of task that needs to be executed in any order.
	 *
	 * A begin and end task are also added for convenience.
	 */
	class Job {
		public:
			/**
			 * Type used to iterate on a Job, yielding job's tasks.
			 */
			typedef std::list<Task*>::const_iterator TaskIterator;

			/**
			 * Job id.
			 */
			virtual JID id() const = 0;
			/**
			 * Adds a task to this Job.
			 *
			 * @param task task to add
			 */
			virtual void add(Task& task) = 0;
			/**
			 * Removes a task from this job.
			 *
			 * @param task task to remove
			 */
			virtual void remove(Task& task) = 0;
			/**
			 * Returns a reference to the internal Tasks list.
			 *
			 * @return current tasks list
			 */
			virtual std::vector<Task*> tasks() const = 0;
			/**
			 * Returns a begin iterator to the job's tasks.
			 *
			 * @return begin iterator
			 */
			virtual TaskIterator begin() const = 0;
			/**
			 * Returns an end iterator to the job's tasks.
			 *
			 * @return end iterator
			 */
			virtual TaskIterator end() const = 0;

			/**
			 * Sets the begin task of the job.
			 *
			 * This task is assumed to be executed before the regular and
			 * unordered task list.
			 *
			 * @param task begin task
			 */
			virtual void setBeginTask(Task& task) = 0;
			/**
			 * Gets a reference to the begin task of this job.
			 *
			 * @return begin task
			 */
			virtual Task& getBeginTask() const = 0;

			/**
			 * Sets the end task of the job.
			 *
			 * This task is assumed to be executed after the regular and
			 * unordered task list.
			 *
			 * @param task end task
			 */
			virtual void setEndTask(Task& task) = 0;
			/**
			 * Gets a reference to the end task of this job.
			 *
			 * @return end task
			 */
			virtual Task& getEndTask() const = 0;

			virtual ~Job() {}
	};

	/**
	 * A list of Jobs.
	 */
	typedef std::vector<std::reference_wrapper<const Job>> JobList;

	/**
	 * Epoch API.
	 *
	 * An Epoch describes an ordered set of Job that needs to be executed
	 * at a given Date.
	 */
	class Epoch {
		public:

			/**
			 * Type used to iterate on an Epoch, yielding epoch's jobs.
			 */
			typedef std::vector<const Job*>::const_iterator JobIterator;

			/**
			 * Submits a new job to this Epoch.
			 *
			 * Jobs within the Epoch are ordered according to the specified
			 * `sub_time_step`. The result of jobs() and the begin() / end()
			 * iterators are consistent with this order. If several jobs are
			 * submitted with equal `sub_time_steps`, their relative ordering
			 * corresponds to the order they are submitted. However it is risky
			 * to rely on this, since float comparison can be approximate.
			 *
			 * @param job job to submit
			 * @param sub_time_step value used to order jobs within the Epoch
			 */
			virtual void submit(const Job& job, SubTimeStep sub_time_step) = 0;

			/**
			 * Submits a list of job to be executed sequentially in this Epoch.
			 *
			 * Jobs within the Epoch are ordered according to the specified
			 * `sub_time_step`. The result of jobs() and the begin() / end()
			 * iterators are consistent with this order. If several jobs are
			 * submitted with equal `sub_time_steps`, their relative ordering
			 * corresponds to the order they are submitted. However it is risky
			 * to rely on this, since float comparison can be approximate.
			 *
			 * @param job_list list of jobs to submit
			 * @param sub_time_step value used to order jobs within the Epoch
			 */
			virtual void submit(JobList job_list, SubTimeStep sub_time_step) = 0;

			/**
			 * Returns a reference to the internal Jobs list.
			 *
			 * @return current jobs list
			 */
			virtual const std::vector<const Job*>& jobs() const = 0;

			/**
			 * Returns a begin iterator to the epoch's jobs.
			 *
			 * @return begin iterator
			 */
			virtual JobIterator begin() const = 0;
			/**
			 * Returns an end iterator to the epoch's jobs.
			 *
			 * @return end iterator
			 */
			virtual JobIterator end() const = 0;
			/**
			 * Returns the current number of jobs in the Epoch.
			 *
			 * @return job count
			 */
			virtual size_t jobCount() = 0;

			/**
			 * Clears the Epoch, removing all jobs currently submitted.
			 */
			virtual void clear() = 0;
			virtual ~Epoch() {};
	};

	/**
	 * Scheduler API.
	 *
	 * The Scheduler can be used to submit jobs to execute punctually at a
	 * given Date or recurrently at a given Period.
	 *
	 * A Runtime can be used to execute jobs submitted to the Scheduler.
	 */
	class Scheduler {
		public:
			/**
			 * Schedule a Job to be executed at the given Date.
			 *
			 * @param date date when the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, const Job& job) = 0;
			/**
			 * Schedule a Job to be executed at the given Period, starting
			 * from the given Date.
			 *
			 * In other terms, the job will be executed at `date`,
			 * `date+period` and `date+n*period` for any integer `n`.
			 *
			 * @param date date when the job must be executed at first
			 * @param period period at which the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, Period period, const Job& job) = 0;

			/**
			 * Schedules a Job to be executed at the given Period, starting
			 * from the given Date, until the end Date.
			 *
			 * In other terms, the job will be executed at `date`,
			 * `date+period` and `date+n*period` for any integer `n` such
			 * that `date+n*period < end`.
			 *
			 * @param date date when the job must be executed at first
			 * @param end date until when the job must be executed
			 * @param period period at which the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, Date end, Period period, const Job& job) = 0;

			/**
			 * Schedules a Job list to be executed sequentially at the given
			 * Date.
			 *
			 * @param date date when the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, JobList job) = 0;

			/**
			 * Schedules a Job list to be executed sequentially at the given
			 * Period, starting from the given Date.
			 *
			 * In other terms, the job will be executed at `date`,
			 * `date+period` and `date+n*period` for any integer `n`.
			 *
			 * @param date date when the job must be executed at first
			 * @param period period at which the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, Period period, JobList job) = 0;
			/**
			 * Schedules a Job list to be executed sequentially at the given
			 * Period, starting from the given Date, until the end Date.
			 *
			 * In other terms, the job will be executed at `date`,
			 * `date+period` and `date+n*period` for any integer `n` such
			 * that `date+n*period < end`.
			 *
			 * @param date date when the job must be executed at first
			 * @param end date until when the job must be executed
			 * @param period period at which the job must be executed
			 * @param job job to execute
			 */
			virtual void schedule(Date date, Date end, Period period, JobList job) = 0;

			/**
			 * Builds an Epoch that correspond to the specified date.
			 *
			 * All jobs currently scheduled at the specified time step,
			 * according to the rules of the schedule() functions, will be
			 * submitted to the given epoch. Jobs are ordered by Date within
			 * the built epoch. If jobs are submitted with the same Date, the
			 * execution order is undefined.
			 *
			 * @see Epoch::submit()
			 * @param step time step to build 
			 * @param epoch output epoch
			 */
			virtual void build(TimeStep step, Epoch& epoch) const = 0;

			virtual ~Scheduler() {}
	};
}}}
#endif
