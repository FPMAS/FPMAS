#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include <vector>

namespace FPMAS {
	typedef unsigned long Period;
	typedef unsigned long Date;
	typedef size_t JID;

	enum ScheduleOverlapPolicy {
		STACK,
		OVERRIDE
	};

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
				virtual void submit(Job*) = 0;
				virtual const std::vector<Job*>& jobs() const = 0;

				virtual void clear() = 0;
				virtual ~Epoch() {};
		};

		class Scheduler {
			public:
				virtual Job& schedule(Date date, ScheduleOverlapPolicy policy) = 0;
				virtual Job& schedule(Date date, Period period, ScheduleOverlapPolicy policy) = 0;

				virtual const Epoch& epoch(Date date) const = 0;
				virtual ~Scheduler() {}
		};

	}
}
#endif
