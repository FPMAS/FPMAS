#include "fpmas/scheduler/scheduler.h"

#include "scheduler/mock_scheduler.h"

using fpmas::scheduler::Epoch;

using ::testing::UnorderedElementsAre;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

class EpochTest : public ::testing::Test {

	protected:
		Epoch epoch;
};

TEST_F(EpochTest, submit_same_substep) {
	std::array<MockJob, 6> jobs;
	for(auto& job : jobs)
		epoch.submit(job, 0);

	ASSERT_THAT(epoch.jobs(), ElementsAre(
		&jobs[0], &jobs[1], &jobs[2], &jobs[3], &jobs[4], &jobs[5]
		));
}

TEST_F(EpochTest, submit_with_substeps) {
	std::array<MockJob, 4> jobs;
	epoch.submit(jobs[0], .34);
	epoch.submit(jobs[1], .1);
	epoch.submit(jobs[2], .5);
	epoch.submit(jobs[3], .1);

	ASSERT_THAT(epoch.jobs(), ElementsAre(
		&jobs[1], &jobs[3], &jobs[0], &jobs[2]
		));
}

TEST_F(EpochTest, submit_lists) {
	std::array<MockJob, 4> jobs;
	epoch.submit(jobs[0], .2);
	epoch.submit({jobs[1], jobs[2]}, .2);
	epoch.submit(jobs[0], .2);
	epoch.submit(jobs[3], .1);

	ASSERT_THAT(epoch.jobs(), ElementsAre(
		&jobs[3], &jobs[0], &jobs[1], &jobs[2], &jobs[0]
		));
}

TEST_F(EpochTest, clear) {
	std::array<MockJob, 6> jobs;
	for(auto& job : jobs)
		epoch.submit(job, 0);

	epoch.clear();
	ASSERT_THAT(epoch.jobs(), IsEmpty());
}
