#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include <vector>

namespace FPMAS {
	typedef unsigned long Period;
	typedef unsigned long Date;
	typedef size_t JID;

	/*
	 *enum ScheduleOverlapPolicy {
	 *    STACK,
	 *    OVERRIDE
	 *};
	 */

	namespace api::scheduler {

		class Task {
			public:
				virtual void execute() = 0;
				virtual ~Task() {}
		};

		class Job {
			public:
				virtual JID id() const = 0;
				virtual void add(Task*) = 0;
				virtual const std::vector<Task*>& tasks() const = 0;

				virtual void setBegin(Task*) = 0;
				virtual Task* getBegin() const = 0;

				virtual void setEnd(Task*) = 0;
				virtual Task* getEnd() const = 0;

				virtual ~Job() {}
		};

		class Epoch {
			public:
				typedef std::vector<Job*>::iterator iterator;

				virtual void submit(Job*) = 0;
				virtual const std::vector<Job*>& jobs() const = 0;
				virtual iterator begin() = 0;
				virtual iterator end() = 0;
				virtual size_t jobCount() = 0;

				virtual void clear() = 0;
				virtual ~Epoch() {};
		};

		class Scheduler {
			public:
				virtual void schedule(Date date, Job*) = 0;
				virtual void schedule(Date date, Period period, Job*) = 0;
				virtual void schedule(Date date, Date end, Period period, Job*) = 0;

				virtual void build(Date date, Epoch&) const = 0;
				virtual ~Scheduler() {}
		};

	}
}
#endif
