#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include <vector>

namespace FPMAS {
	typedef unsigned long Period;
	typedef unsigned long Date;
	typedef size_t JID;

	namespace api::scheduler {

		class Task {
			public:
				virtual void execute() = 0;
				virtual ~Task() {}
		};

		class Job {
			public:
				typedef std::vector<Task*>::iterator TaskIterator;

				virtual JID id() const = 0;
				virtual void add(Task*) = 0;
				virtual const std::vector<Task*>& tasks() const = 0;
				virtual TaskIterator begin() = 0;
				virtual TaskIterator end() = 0;

				virtual void setBeginTask(Task*) = 0;
				virtual Task* getBeginTask() const = 0;

				virtual void setEndTask(Task*) = 0;
				virtual Task* getEndTask() const = 0;

				virtual ~Job() {}
		};

		class Epoch {
			public:
				typedef std::vector<Job*>::iterator JobIterator;

				virtual void submit(Job*) = 0;
				virtual const std::vector<Job*>& jobs() const = 0;
				virtual JobIterator begin() = 0;
				virtual JobIterator end() = 0;
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
