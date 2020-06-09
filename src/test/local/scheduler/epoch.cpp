#include "scheduler/scheduler.h"

#include "../mocks/scheduler/mock_scheduler.h"

using FPMAS::scheduler::Epoch;

using ::testing::UnorderedElementsAre;
using ::testing::IsEmpty;

class EpochTest : public ::testing::Test {

	protected:
		Epoch epoch;
};

TEST_F(EpochTest, submit) {
	std::array<MockJob, 6> jobs;
	for(auto& job : jobs)
		epoch.submit(job);

	ASSERT_THAT(epoch.jobs(), UnorderedElementsAre(
		&jobs[0], &jobs[1], &jobs[2], &jobs[3], &jobs[4], &jobs[5]
		));

}

TEST_F(EpochTest, clear) {
	std::array<MockJob, 6> jobs;
	for(auto& job : jobs)
		epoch.submit(job);

	epoch.clear();
	ASSERT_THAT(epoch.jobs(), IsEmpty());
}
