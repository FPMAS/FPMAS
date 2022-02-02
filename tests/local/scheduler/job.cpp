#include "fpmas/scheduler/scheduler.h"

#include "scheduler/mock_scheduler.h"

using fpmas::scheduler::Job;

using namespace testing;

class JobTest : public ::testing::Test {
	protected:
		Job job;
};

TEST_F(JobTest, id) {
	Job job_2;
	ASSERT_NE(job.id(), job_2.id());
}

TEST_F(JobTest, add) {
	std::array<NiceMock<MockTask>, 6> tasks;
	
	for(auto& task : tasks)
		job.add(task);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[3], &tasks[4], &tasks[5]
				));
}

TEST_F(JobTest, remove) {
	std::array<NiceMock<MockTask>, 6> tasks;
	std::array<std::list<fpmas::api::scheduler::Task*>::iterator, 6> task_pos;
	for(std::size_t i = 0; i < 6; i++) {
		ON_CALL(tasks[i], setJobPos)
			.WillByDefault(Invoke([&task_pos, i] (
							fpmas::api::scheduler::JID,
							std::list<fpmas::api::scheduler::Task*>::iterator pos
							) {
						task_pos[i] = pos;
						}
						));
		ON_CALL(tasks[i], getJobPos)
			.WillByDefault(ReturnPointee(&task_pos[i]));
	}
	
	for(auto& task : tasks)
		job.add(task);

	job.remove(tasks[3]);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[4], &tasks[5]
				));
}

TEST_F(JobTest, begin) {
	MockTask begin;

	job.setBeginTask(begin);
	ASSERT_THAT(job.getBeginTask(), Ref(begin));

	// Begin should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}

TEST_F(JobTest, end) {
	MockTask end;

	job.setEndTask(end);
	ASSERT_THAT(job.getEndTask(), Ref(end));

	// End should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}
