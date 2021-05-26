#include "fpmas/scheduler/scheduler.h"

#include "scheduler/mock_scheduler.h"

using fpmas::scheduler::Job;

using ::testing::Ref;
using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;

class JobTest : public ::testing::Test {
	protected:
		Job job;
};

TEST_F(JobTest, id) {
	Job job_2;
	ASSERT_NE(job.id(), job_2.id());
}

TEST_F(JobTest, add) {
	std::array<MockTask, 6> tasks;
	
	for(auto& task : tasks)
		job.add(task);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[3], &tasks[4], &tasks[5]
				));
}

TEST_F(JobTest, remove) {
	std::array<MockTask, 6> tasks;
	
	for(auto& task : tasks)
		job.add(task);

	job.remove(tasks[3]);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[4], &tasks[5]
				));
}

TEST_F(JobTest, remove_not_contained) {
	std::array<MockTask, 6> tasks;
	
	for(auto& task : tasks)
		job.add(task);

	MockTask other_task;
	job.remove(other_task);

	ASSERT_THAT(job.tasks(), UnorderedElementsAre(
				&tasks[0], &tasks[1], &tasks[2], &tasks[3], &tasks[4], &tasks[5]
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
