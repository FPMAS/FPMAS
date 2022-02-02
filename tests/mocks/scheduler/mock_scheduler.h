#ifndef MOCK_SCHEDULER_H
#define MOCK_SCHEDULER_H

#include "gmock/gmock.h"
#include "fpmas/api/scheduler/scheduler.h"

class MockTask : public fpmas::api::scheduler::Task {
	public:
		MOCK_METHOD(void, setJobPos, (
					fpmas::api::scheduler::JID,
					std::list<fpmas::api::scheduler::Task*>::iterator
					), (override));
		MOCK_METHOD(std::list<fpmas::api::scheduler::Task*>::iterator,
				getJobPos, (fpmas::api::scheduler::JID),
				(const, override)
				);
		MOCK_METHOD(void, run, (), (override));
};

class MockJob : public fpmas::api::scheduler::Job {
	public:
		MOCK_METHOD(fpmas::api::scheduler::JID, id, (), (const, override));
		MOCK_METHOD(void, add, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(void, remove, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(std::vector<fpmas::api::scheduler::Task*>, tasks, (), (const, override));
		MOCK_METHOD(TaskIterator, begin, (), (const, override));
		MOCK_METHOD(TaskIterator, end, (), (const, override));

		MOCK_METHOD(void, setBeginTask, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(fpmas::api::scheduler::Task&, getBeginTask, (), (const, override));

		MOCK_METHOD(void, setEndTask, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(fpmas::api::scheduler::Task&, getEndTask, (), (const, override));

};

class MockEpoch : public fpmas::api::scheduler::Epoch {
	public:
		MOCK_METHOD(void, submit, (const fpmas::api::scheduler::Job&, fpmas::api::scheduler::SubTimeStep), (override));
		MOCK_METHOD(void, submit, (fpmas::api::scheduler::JobList, fpmas::api::scheduler::SubTimeStep), (override));
		MOCK_METHOD(const std::vector<const fpmas::api::scheduler::Job*>&, jobs, (), (const, override));
		MOCK_METHOD(JobIterator, begin, (), (const, override));
		MOCK_METHOD(JobIterator, end, (), (const, override));
		MOCK_METHOD(size_t, jobCount, (), (override));
		MOCK_METHOD(void, clear, (), (override));
};

class MockScheduler : public fpmas::api::scheduler::Scheduler {
	public:
		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, fpmas::api::scheduler::Period, const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, fpmas::api::scheduler::Date, fpmas::api::scheduler::Period, const fpmas::api::scheduler::Job&), (override));

		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, fpmas::api::scheduler::JobList), (override));
		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, fpmas::api::scheduler::Period, fpmas::api::scheduler::JobList), (override));
		MOCK_METHOD(void, schedule, (fpmas::api::scheduler::Date, fpmas::api::scheduler::Date, fpmas::api::scheduler::Period, fpmas::api::scheduler::JobList), (override));

		MOCK_METHOD(void, build, (fpmas::api::scheduler::TimeStep, fpmas::api::scheduler::Epoch&), (const, override));
};

#endif
