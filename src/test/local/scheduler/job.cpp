#include "scheduler/scheduler.h"

#include "../mocks/scheduler/mock_scheduler.h"

using FPMAS::scheduler::Job;

using ::testing::UnorderedElementsAreArray;
using ::testing::IsEmpty;

class JobTest : public ::testing::Test {
	protected:
		const FPMAS::JID id = 236;
		Job job {id};
};

TEST_F(JobTest, id) {
	ASSERT_EQ(job.id(), id);
}

TEST_F(JobTest, add) {
	std::array<MockTask*, 6> tasks;
	for(auto task : tasks)
		job.add(task);

	ASSERT_THAT(job.tasks(), UnorderedElementsAreArray(tasks));
}

TEST_F(JobTest, begin) {
	MockTask begin;

	job.setBegin(&begin);
	ASSERT_EQ(job.getBegin(), &begin);

	// Begin should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}

TEST_F(JobTest, end) {
	MockTask end;

	job.setEnd(&end);
	ASSERT_EQ(job.getEnd(), &end);

	// End should not be part of the regular task list
	ASSERT_THAT(job.tasks(), IsEmpty());
}
