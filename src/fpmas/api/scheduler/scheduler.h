#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include "fpmas/api/graph/distributed_node.h"

namespace fpmas { namespace api { namespace scheduler {
	/**
	 * Type used to define a time period.
	 */
	typedef unsigned long Period;
	/**
	 * Type used to define a particular date.
	 */
	typedef unsigned long Date;
	/**
	 * Job ID type.
	 */
	typedef size_t JID;

	/**
	 * Task API.
	 */
	class Task {
		public:
			/**
			 * Runs the task.
			 */
			virtual void run() = 0;
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
			typedef std::vector<Task*>::const_iterator TaskIterator;

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
			virtual const std::vector<Task*>& tasks() const = 0;
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
			virtual void setEndTask(Task&) = 0;
			/**
			 * Gets a reference to the end task of this job.
			 *
			 * @return end task
			 */
			virtual Task& getEndTask() const = 0;

			virtual ~Job() {}
	};

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
			 * Jobs are assumed to be executed in the order they are added
			 * to the Epoch.  The result of jobs() and the begin() / end()
			 * iterators are consistent with this order.
			 *
			 * @param job job to submit
			 */
			virtual void submit(const Job& job) = 0;

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
			 * Schedule a Job to be executed at the given Period, starting
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
			 * Builds an Epoch that correspond to the specified date.
			 *
			 * All jobs currently scheduled at the specified date,
			 * according to the rules of the schedule() functions, will be
			 * submitted to the given epoch.
			 *
			 * @see Epoch::submit()
			 * @param date input date
			 * @param epoch output epoch
			 */
			virtual void build(Date date, Epoch& epoch) const = 0;

			virtual ~Scheduler() {}
	};
}}}
#endif
