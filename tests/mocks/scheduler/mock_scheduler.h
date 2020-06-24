#ifndef MOCK_SCHEDULER_H
#define MOCK_SCHEDULER_H

#include "gmock/gmock.h"
#include "api/scheduler/scheduler.h"

class MockTask : public fpmas::api::scheduler::Task {
	public:
		MOCK_METHOD(void, run, (), (override));
};

class MockJob : public fpmas::api::scheduler::Job {
	public:
		MOCK_METHOD(fpmas::JID, id, (), (const, override));
		MOCK_METHOD(void, add, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(void, remove, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(const std::vector<fpmas::api::scheduler::Task*>&, tasks, (), (const, override));
		MOCK_METHOD(TaskIterator, begin, (), (const, override));
		MOCK_METHOD(TaskIterator, end, (), (const, override));

		MOCK_METHOD(void, setBeginTask, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(fpmas::api::scheduler::Task&, getBeginTask, (), (const, override));

		MOCK_METHOD(void, setEndTask, (fpmas::api::scheduler::Task&), (override));
		MOCK_METHOD(fpmas::api::scheduler::Task&, getEndTask, (), (const, override));

};

class MockEpoch : public fpmas::api::scheduler::Epoch {
	public:
		MOCK_METHOD(void, submit, (const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(const std::vector<const fpmas::api::scheduler::Job*>&, jobs, (), (const, override));
		MOCK_METHOD(JobIterator, begin, (), (const, override));
		MOCK_METHOD(JobIterator, end, (), (const, override));
		MOCK_METHOD(size_t, jobCount, (), (override));
		MOCK_METHOD(void, clear, (), (override));
};

class MockScheduler : public fpmas::api::scheduler::Scheduler {
	public:
		MOCK_METHOD(void, schedule, (fpmas::Date, const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (fpmas::Date, fpmas::Period, const fpmas::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (fpmas::Date, fpmas::Date, fpmas::Period, const fpmas::api::scheduler::Job&), (override));

		MOCK_METHOD(void, build, (fpmas::Date, fpmas::api::scheduler::Epoch&), (const, override));
};

#endif
