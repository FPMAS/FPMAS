#ifndef MOCK_SCHEDULER_H
#define MOCK_SCHEDULER_H

#include "gmock/gmock.h"
#include "api/scheduler/scheduler.h"

class MockTask : public FPMAS::api::scheduler::Task {
	public:
		MOCK_METHOD(void, run, (), (override));
};

class MockJob : public FPMAS::api::scheduler::Job {
	public:
		MOCK_METHOD(FPMAS::JID, id, (), (const, override));
		MOCK_METHOD(void, add, (FPMAS::api::scheduler::Task&), (override));
		MOCK_METHOD(void, remove, (FPMAS::api::scheduler::Task&), (override));
		MOCK_METHOD(const std::vector<FPMAS::api::scheduler::Task*>&, tasks, (), (const, override));
		MOCK_METHOD(TaskIterator, begin, (), (const, override));
		MOCK_METHOD(TaskIterator, end, (), (const, override));

		MOCK_METHOD(void, setBeginTask, (FPMAS::api::scheduler::Task&), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Task&, getBeginTask, (), (const, override));

		MOCK_METHOD(void, setEndTask, (FPMAS::api::scheduler::Task&), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Task&, getEndTask, (), (const, override));

};

class MockEpoch : public FPMAS::api::scheduler::Epoch {
	public:
		MOCK_METHOD(void, submit, (const FPMAS::api::scheduler::Job&), (override));
		MOCK_METHOD(const std::vector<const FPMAS::api::scheduler::Job*>&, jobs, (), (const, override));
		MOCK_METHOD(JobIterator, begin, (), (const, override));
		MOCK_METHOD(JobIterator, end, (), (const, override));
		MOCK_METHOD(size_t, jobCount, (), (override));
		MOCK_METHOD(void, clear, (), (override));
};

class MockScheduler : public FPMAS::api::scheduler::Scheduler {
	public:
		MOCK_METHOD(void, schedule, (FPMAS::Date, const FPMAS::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (FPMAS::Date, FPMAS::Period, const FPMAS::api::scheduler::Job&), (override));
		MOCK_METHOD(void, schedule, (FPMAS::Date, FPMAS::Date, FPMAS::Period, const FPMAS::api::scheduler::Job&), (override));

		MOCK_METHOD(void, build, (FPMAS::Date, FPMAS::api::scheduler::Epoch&), (const, override));
};

#endif
