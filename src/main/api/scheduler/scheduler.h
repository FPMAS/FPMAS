#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include "api/graph/parallel/distributed_node.h"

namespace FPMAS {
	typedef unsigned long Period;
	typedef unsigned long Date;
	typedef size_t JID;

	namespace api::scheduler {

		class Task {
			public:
				virtual void run() = 0;
				virtual ~Task() {}
		};

		template<typename T>
		class NodeTask : public Task {
			public:
				virtual api::graph::parallel::DistributedNode<T>* node() = 0;
		};

		class Job {
			public:
				typedef std::vector<Task*>::const_iterator TaskIterator;

				virtual JID id() const = 0;
				virtual void add(Task&) = 0;
				virtual void remove(Task&) = 0;
				virtual const std::vector<Task*>& tasks() const = 0;
				virtual TaskIterator begin() const = 0;
				virtual TaskIterator end() const = 0;

				virtual void setBeginTask(Task&) = 0;
				virtual Task& getBeginTask() const = 0;

				virtual void setEndTask(Task&) = 0;
				virtual Task& getEndTask() const = 0;

				virtual ~Job() {}
		};

		class Epoch {
			public:
				typedef std::vector<const Job*>::const_iterator JobIterator;

				virtual void submit(const Job&) = 0;
				virtual const std::vector<const Job*>& jobs() const = 0;
				virtual JobIterator begin() const = 0;
				virtual JobIterator end() const = 0;
				virtual size_t jobCount() = 0;

				virtual void clear() = 0;
				virtual ~Epoch() {};
		};

		class Scheduler {
			public:
				virtual void schedule(Date date, const Job&) = 0;
				virtual void schedule(Date date, Period period, const Job&) = 0;
				virtual void schedule(Date date, Date end, Period period, const Job&) = 0;

				virtual void build(Date date, Epoch&) const = 0;
				virtual ~Scheduler() {}
		};

	}
}
#endif
